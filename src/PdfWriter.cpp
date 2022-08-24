#include "include/pdfWriter.h"
#include <iostream>
#include <qpdfwriter.h>
#include <fstream>
#include <regex>
#include <math.h>
#include <iomanip>
#include <Eigen/Core>
#include <Eigen/LU>
#include<qtextdocument.h>
#include <qmessagebox.h>
#include <qlabel.h>
#include <QBoxLayout.h>
#include <QPushButton.h>
#include <cmath>
#include <QPrinter.h>
#include <QPrintDialog.h>
#include <QTextEdit.h>
#include <winspool.h>
#include <wingdi.h>
#include "include/MainWindow.h"

#define round1(a)(float(int(a*10))/10)

#if defined(unix)
#define min(a,b)({a > b ? a : b;})
#include <sys/stat.h>
#else
#include "windows.h"
#include <winbase.h>
#endif

# define M_PI           3.14159265358979323846
# define HEAD_SIZE 390

void dispatchToMainThread(std::function<void()> callback);

std::filesystem::path PdfWriter::output;
bool PdfWriter::debug = false;
bool PdfWriter::manual = false;

bool PdfWriter::printPdf(std::filesystem::path path) {
		std::cout << GetLastError() << std::endl;
	try {
		std::cout << "trying to print" << std::endl;
		LPDEVMODE pDevMode;
		DWORD dwNeeded, dwRet;
		HANDLE hPrinter;
		char pDevice[] = "\\\\server.scs.local\\OKI MC873";
		//char *pDevice = (char*)QPrinterInfo::defaultPrinterName().toStdString().c_str();
		std::cout <<"Printer: " <<  pDevice << std::endl;
		if (!OpenPrinter(pDevice, &hPrinter, NULL)) {
			std::cout << "failed :(" << std::endl;
			std::cout << GetLastError() << std::endl;
			return false;
		}

		dwNeeded = DocumentProperties(NULL, hPrinter, pDevice, NULL, NULL, 0);
		pDevMode = (LPDEVMODE)malloc(dwNeeded);
		dwRet = DocumentProperties(NULL, hPrinter, pDevice, pDevMode, NULL, DM_OUT_BUFFER);
		if (dwRet != IDOK) {
			free(pDevMode);
			ClosePrinter(hPrinter);
			return false;
		}

		if (pDevMode->dmFields & DM_ORIENTATION) {
			pDevMode->dmOrientation = DMORIENT_LANDSCAPE;
		}

		dwRet = DocumentProperties(NULL, hPrinter, pDevice, pDevMode, pDevMode, DM_IN_BUFFER | DM_OUT_BUFFER);


		if (dwRet != IDOK) {
			free(pDevMode);
			return false;
		}

		DOC_INFO_1 doc_info = {0};

		doc_info.pDocName = (LPSTR)"EH35.pdf";
		doc_info.pOutputFile = pDevice;
		//doc_info.pDatatype = (LPTSTR) NULL;
		doc_info.pDatatype = (LPSTR)"raw";

		std::cout << "before start" << GetLastError() << std::endl;
		DWORD jobid = StartDocPrinter(hPrinter, 1, (LPBYTE)&doc_info);
		std::cout << "after start" << GetLastError() << std::endl;
		std::cout << "jobid: " << jobid << std::endl;

		if ( jobid != 0 ) {
			DWORD written;
			DWORD dwNumBytes = lstrlen(pDevice);
			
			WritePrinter( hPrinter, (void*) pDevice,
			dwNumBytes, &written );
		}

		EndDocPrinter(hPrinter);
		ClosePrinter(hPrinter);

		//std::cout << (LPCSTR)pDevice << std::endl;
		//HDC hdcPrint = CreateDCA("PressPrograms", (LPCSTR)pDevice, NULL, pDevMode);
		//DOCINFO info;
		//memset(&info, 0, sizeof(info));
		//info.cbSize = sizeof(info);
		//StartDoc(hdcPrint, &info);
		//StartPage(hdcPrint);
		//Rectangle(hdcPrint, 100, 100, 200, 200);
		//EndPage(hdcPrint);
		//EndDoc(hdcPrint);
		//DeleteDC(hdcPrint);
		//ClosePrinter(hPrinter);
		std::cout << "Code: " << GetLastError() << std::endl;
		return true;

	}
	catch (std::exception e) {
		std::cout << e.what() << std::endl;
		return false;
	}
}

void PdfWriter::makePdf(const std::filesystem::path& input, QWidget* window) {
	std::cout << "Making " << input.string() << std::endl;
	try {
		PressDrawing* pressDrawing = new PressDrawing(input);
		drawPdf(pressDrawing);
		delete pressDrawing;
	}
	catch (PdfWriter::BadFileException e) {
	dispatchToMainThread([e, window]() {
		QDialog* test = new QDialog(window);
		test->setFixedSize(300, 100);
		test->setModal(false);
		test->setWindowTitle("Failed");
		QBoxLayout* l = new QBoxLayout(QBoxLayout::Direction::TopToBottom, test);
		QLabel* label = new QLabel(QString::fromStdString(e.str()));
		label->setWordWrap(true);
		QPushButton* button = new QPushButton("Continue");
		l->addWidget(label);
		l->addWidget(button);
		QObject::connect(button, &QPushButton::clicked, [label, button, l, test]() mutable {
			test->done(0);
			delete label, button, l, test;
						 });
	test->show(); });
	}
}

std::pair<std::string, std::string> PdfWriter::split(std::string str, const char* del) {
	std::pair<std::string, std::string> output;
	output.first = str.substr(0, str.find(del));
	output.second = str.substr(str.find(del) + 1, str.length() - str.find(del));
	return output;
}

PdfWriter::PressDrawing::PressDrawing(std::filesystem::path path) {
	std::ifstream file;
	file.open(path);
	if (!file) {
		throw PdfWriter::BadFileException(PdfWriter::BadFileException::MISSING, path);
	}
	this->name = path.stem().string();
	std::string line;
	std::regex r("\\[Block\\d+\\]");
	bool skipping = false;
	std::getline(file, line);
	int totalHeaders = 0;
	if (line != "[ScreenMat]")
		throw PdfWriter::BadFileException(PdfWriter::BadFileException::CORRUPT, path);
	this->path = path;
	while (std::getline(file, line)) {
		if (std::regex_search(line, r)) {
			break;
		}
		if (std::regex_search(line, std::regex("\\[MTool\\]"))) {
			skipping = true;
			continue;
		}
		if (line.starts_with("[") && line.ends_with("]")) {
			skipping = false;
			totalHeaders++;
			continue;
		}
		if (skipping)
			continue;
		std::pair<std::string, std::string> strs = PdfWriter::split(line, "=");
		if (strs.first == "MaterialName")
			this->drawing_no = strs.second;
		else if (strs.first == "Width")
			this->width = std::stoi(strs.second);
		else if (strs.first == "Length")
			this->length = std::stoi(strs.second);
		else if (strs.first == "Border_L")
			this->left_border = std::stof(strs.second);
		else if (strs.first == "Border_T")
			this->top_border = std::stof(strs.second);
		else if (strs.first == "Border_R")
			this->right_border = std::stof(strs.second);
		else if (strs.first == "Border_B")
			this->bottom_border = std::stof(strs.second);
		else if (strs.first == "Overlap")
			this->overlap = std::stoi(strs.second);
		else if (strs.first == "RebateWidth")
			this->rebate_width = std::stoi(strs.second);
		else if (strs.first == "RebatDepth")
			this->rebate_depth = std::stoi(strs.second);
		else if (strs.first == "Block Count")
			this->block_count = std::stoi(strs.second);
		else if (strs.first == "TopHook")
			this->top_hook = strs.second;
		else if (strs.first == "BottomHook")
			this->bot_hook = strs.second;
		else if (strs.first == "Thickness")
			this->thickness = std::stoi(strs.second);
		else if (strs.first == "CAD_NO")
			this->material = strs.second;
		else if (strs.first == "ORDER_NO") {
			std::smatch shapeMatch;
			std::smatch digitMatch;
			std::regex_search(strs.second, shapeMatch, std::regex("[a-zA-Z]{2,}"));
			std::string shape = shapeMatch[0];
			std::regex_search(strs.second, digitMatch, std::regex("[0-9.]+ ?(X ?[0-9.]+)?"));
			this->cutter_size = digitMatch[0];
			if (shape == "DIA" || (shape == "" && digitMatch.prefix() == "�"))
				this->aperture_pattern = PdfWriter::AperturePattern::ROUND;
			else if (shape == "SL")
				this->aperture_pattern = PdfWriter::AperturePattern::SLOTT;
			else if (shape == "ST")
				this->aperture_pattern = PdfWriter::AperturePattern::SLOTL;
			else
				this->aperture_pattern = PdfWriter::AperturePattern::SQUARE;
		}
		else if (strs.first == "DATE")
			this->date = strs.second;
		else if (strs.first == "CUSTOMER")
			this->customer = strs.second;
		else if (strs.first == "Understrips")
			this->understrips = strs.second;
		else if (strs.first == "Designer")
			this->drawn_by = strs.second;
		else if (strs.first == "Note1")
			this->notes[0] = strs.second;
		else if (strs.first == "Note2")
			this->notes[1] = strs.second;
		else if (strs.first == "Note3")
			this->notes[2] = strs.second;
		else if (strs.first == "Note4")
			this->notes[3] = strs.second;
		else if (strs.first == "Pattern")
			this->pattern = strs.second;
		else if (strs.first == "ToolNum")
			this->no_of_cutters = std::stoi(strs.second);
		else if (strs.first == "Die_X")
			this->size_x = std::stof(strs.second);
		else if (strs.first == "Die_Y")
			this->size_y = std::stof(strs.second);
		else if (strs.first == "StaggerX")
			this->stagger_x = strs.second == "Yes" ? true : false;
		else if (strs.first == "StaggerY")
			this->stagger_y = strs.second == "Yes" ? true : false;
		else if (strs.first == "Tool_Distance")
			this->cutter_pitch = std::stoi(strs.second);
		else if (strs.first == "Shims")
			this->shim_size = std::stoi(strs.second);
		else if (strs.first == "Space_X")
			this->space_x = std::stof(strs.second);
		else if (strs.first == "Space_Y")
			this->space_y = std::stof(strs.second);
		else if (strs.first == "Plate width")
			this->plate_width = std::stoi(strs.second);
		else if (strs.first == "Overload_Side")
			this->overlap_side = (PdfWriter::OverlapSide)std::stoi(strs.second);
		else if (strs.first == "Tool_Width")
			this->tool_width = std::stoi(strs.second);
		else if (strs.first == "Action_UnPunch" && strs.second == "Yes")
			this->unpunched = new int[4];
		else if (strs.first == "UnPunch_X")
			this->unpunched.value()[0] = std::stoi(strs.second);
		else if (strs.first == "UnPunch_Y")
			this->unpunched.value()[1] = std::stoi(strs.second);
		else if (strs.first == "UnPunch_W")
			this->unpunched.value()[2] = std::stoi(strs.second);
		else if (strs.first == "UnPunch_H")
			this->unpunched.value()[3] = std::stoi(strs.second);
	}
	while (std::regex_search(line, r)) {
		std::getline(file, line);
		PdfWriter::Coord x_coord;
		PdfWriter::Coord y_coord;
		while (!std::regex_search(line, r) && !std::regex_search(line, std::regex("\\[Position\\]"))) {
			std::pair<std::string, std::string> strs = PdfWriter::split(line, "=");
			if (strs.first == "Start_X")
				x_coord.x1 = std::stof(strs.second);
			if (strs.first == "Start_Y")
				y_coord.x1 = std::stof(strs.second);
			if (strs.first == "Width")
				x_coord.x2 = x_coord.x1 + std::stof(strs.second);
			if (strs.first == "Length")
				y_coord.x2 = y_coord.x1 + std::stof(strs.second);

			std::getline(file, line);
		}
		this->x_values.insert(x_coord);
		this->y_values.insert(y_coord);
	}

	std::getline(file, line);
	while (std::regex_search(line, std::regex("Block\\d+=\\d+"))) {
		std::getline(file, line);
		std::vector<Coord> block;
		while (std::regex_search(line, std::regex("\\d+=[0-9.]+, ?[0-9.]+"))) {
			Coord coord;
			line = PdfWriter::split(line, "=").second;
			std::pair<std::string, std::string> pair = PdfWriter::split(line, ", ");
			coord.x1 = std::stof(pair.first);
			coord.x2 = std::stof(pair.second);
			block.push_back(coord);
			std::getline(file, line);
		}
		points.push_back(block);
	}
	std::pair<std::string, std::string> strs = PdfWriter::split(line, "=");
	if (strs.first == "Steps")
		this->press_strokes = std::stoi(strs.second);

	this->no_of_holes = this->press_strokes * this->no_of_cutters;
	switch (this->aperture_pattern) {
		case ROUND:
			this->open_area = (pow((this->size_x / 2.0f), 2) * M_PI * this->no_of_holes) / (this->width * this->length);
			break;
		case SQUARE:
			this->open_area = (double)(this->size_x * this->size_y * this->no_of_holes) / (double)(this->width * this->length);
			break;
		case SLOTT: case SLOTL:
			this->open_area = (double)((this->size_x * this->size_y - ((M_PI * 3 * pow(min(this->size_x, this->size_y), 2)) / 4)) * this->no_of_holes) / (double)(this->width * this->length);
			break;
		default:
			this->open_area = 0.0;

	}
	file.close();
	if (totalHeaders < 5)
		throw PdfWriter::BadFileException(PdfWriter::BadFileException::CORRUPT, path);
}

void PdfWriter::drawPdf(const PdfWriter::PressDrawing* p) {

	std::stringstream titles;
	std::stringstream values;

	titles << "Drawing No.:" << "\n";
	titles << "Customer:" << "\n";
	titles << "Drawn By:" << "\n";
	titles << "Date:" << "\n";
	titles << "\n\n";
	titles << "Material:" << "\n";
	titles << "Thickness:" << "\n";
	titles << "Cutter Size:" << "\n";
	titles << "Length:" << "\n";
	titles << "Width:" << "\n";
	titles << "Overlap:" << "\n";
	titles << "Number of Holes:" << "\n";
	titles << "Open Area:" << "\n";
	titles << "\n\n";
	titles << "Size X:" << "\n";
	titles << "Size Y" << "\n";
	titles << "Quantity of cutter:" << "\n";
	titles << "Cutter Pitch:" << "\n";
	titles << "Shims size:" << "\n\n";
	titles << "Press Strokes:" << "\n";
	titles << "Left Shim:" << "\n";
	titles << "Right Shim:" << "\n";


	values << p->drawing_no << "\n";
	values << p->customer << "\n";
	values << p->drawn_by << "\n";
	values << p->date << "\n";
	values << "\n\n";
	values << p->material << "\n";
	values << p->thickness << "\n";
	values << p->cutter_size << " " << PdfWriter::aperture_initials(p->aperture_pattern) << "\n";
	values << p->length << "\n";
	values << p->width << "\n";
	values << p->overlap << "\n";
	values << p->no_of_holes << "\n";
	values << round1(p->open_area * 100) << "\n";
	values << "\n\n";
	values << p->size_x << "\n";
	values << p->size_y << "\n";
	values << p->no_of_cutters << "\n";
	values << p->cutter_pitch << "\n";
	values << p->shim_size << "\n\n";
	values << p->press_strokes << "\n";

	std::smatch match;
	std::regex_search(p->name, match, std::regex("[A-Z]{1,2}"));
#if (unix)
    const int dir_err = mkdir((output.string() + "\\" + match[0].str()).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err) {
        std::cout << "Error making directory" << std::endl;
        return;
    }
#else
	if (!CreateDirectory((output.string() + "\\" + match[0].str()).c_str(), nullptr))
		if (ERROR_ALREADY_EXISTS != GetLastError()) {
			std::cout << "Error making directory" << std::endl;
			return;
		}

	CopyFile(convertStrTLPTSTR(p->path.string()), convertStrTLPTSTR("T:\\Auto Press Programs\\Press_1\\" + p->path.filename().string()), true);
	CopyFile(convertStrTLPTSTR(p->path.string()), convertStrTLPTSTR("T:\\Auto Press Programs\\Press_2\\" + p->path.filename().string()), true);
#endif
	std::string filename = (std::string(output.string() + "\\" + match[0].str() + "\\" + p->name + (PdfWriter::debug ? " debug" : "") + ".pdf"));
	QPdfWriter* writer = new QPdfWriter(filename.c_str());
	writer->setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape, QMarginsF()));
	writer->setTitle(p->name.c_str());
	QPainter* painter = new QPainter();
	QFont titleFont;
	titleFont.setFamily("neology");
	titleFont.setPixelSize(writer->width() / 35);
	titleFont.setWeight(QFont::Weight::Bold);
	QFont normalFont;
	normalFont.setFamily("neology");
	normalFont.setPixelSize(writer->width() / 75);
	normalFont.setWeight(QFont::Weight::DemiBold);
	normalFont.setLetterSpacing(QFont::SpacingType::PercentageSpacing, 110);
	QFont thinFont;
	thinFont.setFamily("neology");
	thinFont.setPixelSize(writer->width() / 75);
	thinFont.setWeight(QFont::Weight::Light);
	thinFont.setLetterSpacing(QFont::SpacingType::PercentageSpacing, 110);
	QFont italicFont;
	italicFont.setFamily("neology");
	italicFont.setPixelSize(writer->width() / 65);
	italicFont.setWeight(QFont::Weight::DemiBold);
	italicFont.setLetterSpacing(QFont::SpacingType::PercentageSpacing, 110);
	italicFont.setItalic(true);
	if (!painter->begin(writer))
		throw PdfWriter::BadFileException(PdfWriter::BadFileException::UNKNOWN, p->path);
	QBrush normal = painter->brush();

	QRectF leftSide1(0.0, writer->height() * 0.05, writer->width() * 0.14, (double)writer->height() * 0.70);
	QRectF leftSide2(writer->width() * 0.15, writer->height() * 0.05, writer->width() * 0.1, (double)writer->height() * 0.70);
	QRectF notesBox(0.02 * writer->width(), writer->height() * 0.75, writer->width() * 0.25, writer->height() * 0.25);

	QRectF matArea(writer->width() * 0.24, writer->height() * 0.13, writer->width() * 0.50, writer->height() * 0.83);

	QRectF titleBox(writer->width() * 0.21, 0, writer->width() * 0.58, writer->height() * 0.1);
	QRectF rightSide(writer->width() * 0.81, writer->height() * 0.15, writer->width() * 0.19, writer->height() * 0.85);
	QRectF columnLabel(writer->width() * 0.81, writer->height() * 0.1, writer->width() * 0.19, writer->height() * 0.05);

	painter->setFont(normalFont);
	std::stringstream ss;
	for (std::string note : p->notes)
		ss << note << "\n";

	painter->drawText(notesBox, QObject::tr(ss.str().c_str()));

	{
		painter->save();
		QRectF pos(leftSide1.x(), leftSide1.y() + 5 * painter->font().pixelSize(), leftSide2.right(), painter->font().pixelSize() * 2);
		painter->setFont(italicFont);
		painter->drawText(pos, Qt::AlignCenter, "Manufacture details");
		painter->restore();
	}
	{
		painter->save();
		QRectF pos(leftSide1.x(), leftSide1.y() + 16 * painter->font().pixelSize(), leftSide2.right(), painter->font().pixelSize() * 2);
		painter->setFont(italicFont);
		painter->drawText(pos, Qt::AlignCenter, "Aperture");
		painter->restore();
	}

	double multiplier;
	if (matArea.width() / p->width < matArea.height() / p->length) {
		multiplier = (double)(p->width + p->overlap * (1 + (p->overlap_side == PdfWriter::OverlapSide::BOTH))) / (double)p->width;
		multiplier = (double)matArea.width() / (double)p->width;
	}
	else {
		multiplier = (double)matArea.height() / (double)p->length;
	}
	QRectF mat(matArea.center().x() - (p->width * multiplier) / 2,
			   matArea.center().y() - (p->length * multiplier) / 2,
			   ((p->width + p->overlap * (1 + (p->overlap_side == PdfWriter::OverlapSide::BOTH))) * multiplier),
			   (p->length * multiplier));

	painter->setBrush(QColor(166, 202, 240));
	painter->setPen(Qt::red);
	for (Coord x_val : p->x_values) {
		for (Coord y_val : p->y_values) {
			painter->drawRect(QRectF(QPointF(x_val.x1 * multiplier + mat.x(), (p->length - y_val.x1) * multiplier + mat.y()), QPointF(x_val.x2 * multiplier + mat.x(), (p->length - y_val.x2) * multiplier + mat.y())));
		}
	}
	painter->setBrush(normal);
	painter->setPen(Qt::black);


	QRectF xVals(columnLabel.bottomLeft(),
				 QPointF(columnLabel.left() + ((columnLabel.right() - columnLabel.left()) / 2),
				 writer->height()));
	QRectF yVals(QPointF(columnLabel.left() + ((columnLabel.right() - columnLabel.left()) / 2),
				 columnLabel.bottom()),
				 QPointF(writer->width(), writer->height()));

	QPointF pos = QPointF(xVals.topLeft().x(), xVals.topLeft().y() + painter->font().pixelSize());

	std::vector<float> bridges[2];

	painter->save();
	painter->setFont(thinFont);
	int tension_band = 0;
	for (int i = 0; i < p->x_values.size(); i++) {
		painter->setPen(Qt::red);
		std::stringstream x1;
		x1 << "X" << i + 1 << "  " << round1(p->x_values[i].x1);
		painter->drawText(pos, x1.str().c_str());
		pos.setY(pos.y() + painter->font().pixelSize());

		std::stringstream x2;
		x2 << "X" << i + 2 << "  " << round1(p->x_values[i].x2);
		painter->drawText(pos, x2.str().c_str());
		pos.setY(pos.y() + painter->font().pixelSize());

		painter->setPen(Qt::black);
		std::stringstream bridge;
		bridge << "Bridge ";
		float bridgeWidth = 0;
		if (!p->points.empty() && !p->points[i].empty()) {
			Coord first = p->points[i][0];
			for (Coord second : p->points[i]) {
				if (first.x2 == second.x2 && first.x1 != second.x1) {
					bridgeWidth = (float)abs(second.x1 - first.x1) - (float)p->size_x;
					if (p->stagger_x) {
						bridgeWidth = (bridgeWidth - p->size_x) / 2;
					}
					break;
				}
			}
			int points = 1;
			for (Coord second : p->points[i]) {
				if (first.x1 != second.x1 && first.x2 == second.x2) {
					points++;
				}
			}
		}
		bridge << round1(bridgeWidth);
		bridges[0].push_back(bridgeWidth);
		painter->drawText(pos, bridge.str().c_str());
		pos.setY(pos.y() + painter->font().pixelSize());

		if (i < p->x_values.size() - 1) {
			painter->setPen(Qt::blue);
			std::stringstream bar;
			bar << "TEN Band:" << p->x_values[i + 1].x1 - p->x_values[i].x2;
			tension_band = p->x_values[i + 1].x1 - p->x_values[i].x2;
			painter->drawText(pos, bar.str().c_str());
			pos.setY(pos.y() + painter->font().pixelSize());
		}
	}

	int punchedLines = (p->x_values[0].x2 - p->x_values[0].x1 + bridges[0][0]) * p->x_values.size() / (p->no_of_cutters * (p->size_x + bridges[0][0]));
	pos = QPointF(yVals.topLeft().x(), yVals.topLeft().y() + painter->font().pixelSize() * 1.1);

	for (int i = 0; i < p->y_values.size(); i++) {
		painter->setPen(Qt::red);
		std::stringstream y1;
		y1 << "Y" << i + 1 << "  " << round1(p->y_values[i].x1);
		painter->drawText(pos, y1.str().c_str());
		pos.setY(pos.y() + painter->font().pixelSize() * 1.1);

		std::stringstream y2;
		y2 << "Y" << i + 2 << "  " << round1(p->y_values[i].x2);
		painter->drawText(pos, y2.str().c_str());
		pos.setY(pos.y() + painter->font().pixelSize() * 1.1);

		painter->setPen(Qt::black);
		std::stringstream bridge;
		float bridgeHeight = 0;
		if (!p->points.empty() && !p->points[i].empty()) {
			Coord first = p->points[i][0];
			for (Coord second : p->points[i]) {
				if (first.x2 != second.x2 && first.x1 == second.x1) {
					bridgeHeight = (float)abs(second.x2 - first.x2) - (float)p->size_y;
					if (p->stagger_y) {
						bridgeHeight = (bridgeHeight - p->size_y) / 2;
					}
					break;
				}
			}
		}
		bridge << "Bridge ";
		bridge << round1(bridgeHeight);

		painter->drawText(pos, bridge.str().c_str());
		pos.setY(pos.y() + painter->font().pixelSize() * 1.1);
		bridges[1].push_back(bridgeHeight);
		if (i < p->y_values.size() - 1) {
			painter->setPen(Qt::blue);
			std::stringstream bar;
			bar << "SPT Bar:" << p->y_values[i + 1].x1 - p->y_values[i].x2;
			painter->drawText(pos, bar.str().c_str());
			pos.setY(pos.y() + painter->font().pixelSize() * 1.1);
		}
	}
	painter->restore();
	painter->save();
	if (p->overlap_side == PdfWriter::OverlapSide::BOTH) {
		QRectF left(mat.topLeft(), QPointF(p->overlap * multiplier + mat.left(), mat.bottom()));
		painter->fillRect(left, QColor(162, 210, 168));
		painter->drawLine(QLineF(left.topRight(), left.bottomRight()));
		QRectF right(QPointF(mat.right() - p->overlap * multiplier, mat.bottom()), mat.topRight());
		painter->fillRect(right, QColor(162, 210, 168));
		painter->drawLine(QLineF(right.topLeft(), right.bottomLeft()));
	}
	else if (p->overlap_side == PdfWriter::OverlapSide::RIGHT) {
		QRectF temp(QPointF(mat.right() - p->overlap * multiplier, mat.bottom()), mat.topRight());
		painter->fillRect(temp, QColor(162, 210, 168));
		painter->drawLine(QLineF(temp.topLeft(), temp.bottomLeft()));
	}
	painter->restore();
	values << floor(((HEAD_SIZE - ((p->plate_width + p->shim_size) * (p->no_of_cutters - 1) + p->plate_width)) / 2)) << "\n";
	values << ceil(((HEAD_SIZE - ((p->plate_width + p->shim_size) * (p->no_of_cutters - 1) + p->plate_width)) / 2)) << "\n";
	painter->save();
	painter->drawText(leftSide1, Qt::AlignRight, titles.str().c_str());
	painter->setPen(Qt::blue);
	painter->drawText(leftSide2, Qt::AlignLeft, values.str().c_str());
	painter->restore();
	painter->drawRect(mat);
	painter->save();
	QRectF outerRect(p->unpunched.value()[0] * multiplier + mat.x(), p->unpunched.value()[1] * multiplier + mat.y(), p->unpunched.value()[2] * multiplier, p->unpunched.value()[3] * multiplier);
	int dist = mat.height() * 0.02;
	painter->setClipRect(outerRect);
	painter->setPen(QPen(QColor::fromRgb(150, 150, 0)));
	for (int i = 0; i < outerRect.height() + outerRect.width(); i += dist) {
		painter->drawLine(QPoint(outerRect.x(), outerRect.y() + i), QPoint(outerRect.right(), outerRect.y() + i - outerRect.width()));
	}
	painter->restore();
	painter->drawRect(outerRect);
	int repeats = round(punchedLines / (((p->plate_width - p->size_x) + p->shim_size - bridges[0][0]) / (p->size_x + bridges[0][0]) + 1));
	painter->setBrush(QBrush(Qt::yellow));
	if (!debug) {
			for (int first = 0; first < p->points.size(); first++) {
				std::vector<Coord> block = p->points[first];
				for (int second = 0; second < block.size(); second++) {
					Coord coord = block[second];
					switch (p->aperture_pattern) {
						case ROUND:
							if (p->no_of_cutters == 1)
								painter->drawEllipse(QPointF(coord.x1 * multiplier + mat.x(), ((p->length - coord.x2) * multiplier) + mat.y()), (p->size_x * multiplier) / 2, (p->size_y * multiplier) / 2);
							else {
								if (p->no_of_cutters % 2 == 1) {
									for (int i = -(p->no_of_cutters / 2); i <= floor(p->no_of_cutters / 2); i++) {
										painter->drawEllipse(QPointF(coord.x1 * multiplier + mat.x() + ((bridges[0][floor(first / p->y_values.size())] + p->size_x) * i * (punchedLines / repeats)) * multiplier, ((p->length - coord.x2) * multiplier) + mat.y()), (p->size_x * multiplier) / 2, (p->size_y * multiplier) / 2);
									}
								}
								else {
									for (float i = -p->no_of_cutters + 1; i < p->no_of_cutters; i += 2) {
										painter->drawEllipse(QPointF(coord.x1 * multiplier + mat.x() + ((bridges[0][floor(first / p->y_values.size())] + p->size_x) * (i / 2) * (punchedLines / repeats)) * multiplier, ((p->length - coord.x2) * multiplier) + mat.y()), (p->size_x * multiplier) / 2, (p->size_y * multiplier) / 2);
									}
								}
							}
							break;
						case SQUARE:
							if (p->no_of_cutters == 1)
								painter->drawRect(coord.x1 * multiplier + mat.x() - (p->size_x * multiplier / 2), ((p->length - coord.x2) * multiplier) + mat.y() - (p->size_y * multiplier / 2), p->size_x * multiplier, p->size_y * multiplier);
							else {
								if (p->no_of_cutters % 2 == 1) {
									for (int i = -(p->no_of_cutters / 2); i <= floor(p->no_of_cutters / 2); i++) {
										painter->drawRect(coord.x1 * multiplier + mat.x() - (p->size_x * multiplier / 2) + ((bridges[0][floor(first / p->y_values.size())] + p->size_x) * i * punchedLines / repeats) * multiplier, ((p->length - coord.x2) * multiplier) + mat.y() - (p->size_y * multiplier / 2), p->size_x * multiplier, p->size_y * multiplier);
									}
								}
								else {
									for (float i = -p->no_of_cutters + 1.0f; i < p->no_of_cutters; i += 2) {
										painter->drawRect(coord.x1 * multiplier + mat.x() - (p->size_x * multiplier / 2) + ((bridges[0][floor(first / p->y_values.size())] + p->size_x) * (i / 2) * punchedLines / repeats) * multiplier, ((p->length - coord.x2) * multiplier) + mat.y() - (p->size_y * multiplier / 2), p->size_x * multiplier, p->size_y * multiplier);
									}
								}
							}
							break;
                        case SLOTT: case SLOTL:
							if (p->no_of_cutters == 1)
								painter->drawRoundedRect(coord.x1 * multiplier + mat.x() - (p->size_x * multiplier / 2), ((p->length - coord.x2) * multiplier) + mat.y() - (p->size_y * multiplier / 2), p->size_x * multiplier, p->size_y * multiplier, min(p->size_x, p->size_y) * multiplier / 2, min(p->size_x, p->size_y) * multiplier / 2);
							else {
								if (p->no_of_cutters % 2 == 1) {
									for (int i = -(p->no_of_cutters / 2); i <= floor(p->no_of_cutters / 2); i++) {
										painter->drawRoundedRect(coord.x1 * multiplier + mat.x() - (p->size_x * multiplier / 2) + ((bridges[0][floor(first / p->y_values.size())] + p->size_x) * i * punchedLines / repeats) * multiplier, ((p->length - coord.x2) * multiplier) + mat.y() - (p->size_y * multiplier / 2), p->size_x * multiplier, p->size_y * multiplier, min(p->size_x, p->size_y) * multiplier / 4, min(p->size_x, p->size_y) * multiplier / 4);
									}
								}
								else {
									for (float i = -p->no_of_cutters + 1; i < p->no_of_cutters; i += 2) {
										painter->drawRoundedRect(coord.x1 * multiplier + mat.x() - (p->size_x * multiplier / 2) + ((bridges[0][floor(first / p->y_values.size())] + p->size_x) * (i / 2) * punchedLines / repeats) * multiplier, ((p->length - coord.x2) * multiplier) + mat.y() - (p->size_y * multiplier / 2), p->size_x * multiplier, p->size_y * multiplier, min(p->size_x, p->size_y) * multiplier / 4, min(p->size_x, p->size_y) * multiplier / 4);
									}
								}
							}
                            break;
                        default :
                            break;
					}
				}
			}
		
	} else {
		for (int first = 0; first < p->points.size(); first++) {
			std::vector<Coord> block = p->points[first];
			for (PdfWriter::Coord coord : block) {
					switch (p->aperture_pattern) {
					case ROUND:
						painter->drawEllipse(QPointF(coord.x1 * multiplier + mat.x(), ((p->length - coord.x2) * multiplier) + mat.y()), (p->size_x * multiplier) / 2, (p->size_y * multiplier) / 2);
						break;
					case SQUARE:
						painter->drawRect(coord.x1 * multiplier + mat.x() - (p->size_x * multiplier / 2), ((p->length - coord.x2) * multiplier) + mat.y() - (p->size_y * multiplier / 2), p->size_x * multiplier, p->size_y * multiplier);
						break;
                    case SLOTT: case SLOTL:
						painter->drawRoundedRect(coord.x1 * multiplier + mat.x() - (p->size_x * multiplier / 2), ((p->length - coord.x2) * multiplier) + mat.y() - (p->size_y * multiplier / 2), p->size_x * multiplier, p->size_y * multiplier, min(p->size_x, p->size_y) * multiplier / 2, min(p->size_x, p->size_y) * multiplier / 2);
						break;
				}
			}
		}
	}
	painter->setBrush(normal);
	painter->save();
	painter->setFont(thinFont);
	QRectF lengthArea(mat.x() - writer->width() * 0.03, writer->height() * 0.13, writer->width() * 0.03, writer->height() * 0.85);
	QRectF widthArea(writer->width() * 0.24, mat.y() - writer->height() * 0.03, writer->width() * 0.53, writer->height() * 0.03);
	painter->drawText(widthArea, Qt::AlignCenter, ("Width=" + std::to_string(p->width + p->overlap * (1 + (PdfWriter::OverlapSide::BOTH == p->overlap_side)))).c_str());
	QRectF newLengthArea;
	{
		QPointF center = lengthArea.center();
		Eigen::Matrix3f m1, m2, m3, m4;
		m1 << 1, 0, center.x(),
			0, 1, center.y(),
			0, 0, 1;
		m2 << 0, 1, 0,
			-1, 0, 0,
			0, 0, 1;
		m3 << 1, 0, -center.x(),
			0, 1, -center.y(),
			0, 0, 1;
		m4 = m1 * m2 * m3;
		Eigen::Vector3f bottomRight = m4 * Eigen::Vector3f(lengthArea.bottomRight().x(), lengthArea.bottomRight().y(), 1);
		Eigen::Vector3f topLeft = m4 * Eigen::Vector3f(lengthArea.topLeft().x(), lengthArea.topLeft().y(), 1);
		newLengthArea = QRectF(QPointF(topLeft[0], topLeft[1]), QPointF(bottomRight[0], bottomRight[1]));
	}
	QPointF center = newLengthArea.center();
	Eigen::Matrix3f m1, m2, m3, m4;
	m1 << 1, 0, center.x(),
		0, 1, center.y(),
		0, 0, 1;
	m2 << 0, 1, 0,
		-1, 0, 0,
		0, 0, 1;
	m3 << 1, 0, -center.x(),
		0, 1, -center.y(),
		0, 0, 1;
	m4 = (m1 * m2) * m3;
	QPainterPath painterPath();
	painter->setTransform(QTransform(m4(0, 0), m4(1, 0), m4(2, 0), m4(0, 1), m4(1, 1), m4(2, 1), m4(0, 2), m4(1, 2), m4(2, 2)));
	painter->drawText(newLengthArea, Qt::AlignCenter, ("<-Tension->	      Length=" + std::to_string(p->length)).c_str());
	painter->setTransform(QTransform());
	painter->restore();
	painter->setFont(titleFont);
	painter->drawText(titleBox, Qt::AlignCenter, (p->name + " " + std::to_string(p->thickness) + " " + p->cutter_size + " " + p->pattern).c_str());
	painter->setFont(normalFont);

	painter->drawLine(columnLabel.bottomLeft(), columnLabel.bottomRight());
	painter->drawText(QRectF(columnLabel.topLeft(),
					  QPointF(columnLabel.left() + ((columnLabel.right() - columnLabel.left()) / 2),
					  columnLabel.bottom())), Qt::AlignCenter, "X");

	painter->drawText(QRectF(columnLabel.bottomRight(),
					  QPointF(columnLabel.left() + ((columnLabel.right() - columnLabel.left()) / 2),
					  columnLabel.top())), Qt::AlignCenter, "Y");



	painter->end();
	delete writer, painter;
}