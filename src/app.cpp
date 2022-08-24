#include <iostream>
#include <filesystem>
#include <qapplication.h>
#include <QMainWindow>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QLabel>
#include <QBoxLayout>
#include <QMessageBox>
#include <thread>
#include <qtimer.h>
#include "include/PdfWriter.h"
#include "include/MainWindow.h"

MainWindow* MainWindow::window = nullptr;

void watch_directory(LPTSTR path, HANDLE exit);

void dispatchToMainThread(std::function<void()> callback) {
	QTimer* timer = new QTimer();
	timer->moveToThread(qApp->thread());
	timer->setSingleShot(true);
	QObject::connect(timer, &QTimer::timeout, [=]() {
		callback();
		timer->deleteLater();
					 });
	QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}

int runApp() {

	MainWindow* window = MainWindow::getMainWindow();
	QBoxLayout* windowLayout = new QBoxLayout(QBoxLayout::TopToBottom);

	QLabel* description = new QLabel("Program to generate punch press programs from lay files.\nPlease Select the directory containing the lay files.", window);
	QLineEdit* pathEdit = new QLineEdit(window);
	pathEdit->setPlaceholderText("Input Path");
	pathEdit->setText("\\\\server.scs.local\\company\\Auto Press Programs");
	QPushButton* pathButton = new QPushButton("Select input Path", window);
	QWidget* pathWidget = new QWidget();
	QBoxLayout* pathLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	pathLayout->addWidget(pathEdit);
	pathLayout->addWidget(pathButton);
	pathWidget->setLayout(pathLayout);

	QLineEdit* outputEdit = new QLineEdit(window);
	outputEdit->setPlaceholderText("Output Path");
	outputEdit->setText("\\\\server.scs.local\\company\\Drawings\\2. Rubber Screen Cloths\\Punch Program PDF's");
	QPushButton* outputButton = new QPushButton("Select output Path", window);
	QWidget* outputWidget = new QWidget();
	QBoxLayout* outputLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	outputLayout->addWidget(outputEdit);
	outputLayout->addWidget(outputButton);
	outputWidget->setLayout(outputLayout);

	QPushButton* manualButton = new QPushButton("Manual", window);
	manualButton->setCheckable(true);
	manualButton->setChecked(true);
	QPushButton* completeButton = new QPushButton("Start", window);
	completeButton->setCheckable(true);
	windowLayout->addWidget(description);
	windowLayout->addWidget(pathWidget);
	windowLayout->addWidget(outputWidget);
	windowLayout->addWidget(manualButton);
	windowLayout->addWidget(completeButton);

	std::thread *t1;
	HANDLE exit;
	bool manualStop = false;

	window->setLayout(windowLayout);
	QObject::connect(pathButton, &QPushButton::clicked, [pathEdit]() {pathEdit->setText(QFileDialog::getExistingDirectory()); });
	QObject::connect(outputButton, &QPushButton::clicked, [outputEdit]() {outputEdit->setText(QFileDialog::getExistingDirectory()); });
	window->show();
	QObject::connect(completeButton, &QPushButton::clicked, [pathEdit, outputEdit, window, manualButton, completeButton, t1, exit, manualStop]() mutable {
		if (!completeButton->isChecked() && t1->joinable()) {
			SetEvent(exit);
			t1->join();
			delete t1;
		}
		else if (!completeButton->isChecked() && !manualStop) {
			manualStop = true;
		}
		std::filesystem::path path(pathEdit->text().toStdString());
		std::filesystem::path output(outputEdit->text().toStdString());
		PdfWriter::setOutput(output);
		try {
		if (!std::filesystem::exists(output) || !std::filesystem::is_directory(output)) {
				QMessageBox::warning(window, "Empty Directory", "The output Directory is invalid, Please pick again.");
				return;
			}
		if (!std::filesystem::exists(path) || !std::filesystem::is_directory(output)) {
				QMessageBox::warning(window, "Empty Directory", "The input Directory is invalid, Please pick again.");
				return;
			}
		if (std::filesystem::is_empty(path)) {
				QMessageBox::warning(window, "Empty Directory", "The input Directory is empty, Please pick again.");
				return;
			}
			manualStop = false;
			if (manualButton->isChecked() && completeButton->isChecked()) {
					for (const std::filesystem::directory_entry& p : std::filesystem::directory_iterator(path)) {
						if (manualStop)
							break;
						if (p.path().has_extension() && p.path().extension() == ".lay") {
							PdfWriter::makePdf(p, window);
							qApp->processEvents();
						}
					}
					QMessageBox::information(window, "Complete", "All files have been completed");
					completeButton->setChecked(false);
			}
			else if (completeButton->isChecked()) {
				exit = CreateEvent(NULL, false, false, NULL);
				t1 = new std::thread(watch_directory, PdfWriter::convertStrTLPTSTR(pathEdit->text().toStdString()), exit);
			}
		}
		catch (std::filesystem::filesystem_error e) {
			QMessageBox::warning(window, "filesystem error", (std::string("The filesystem threw an error.\n") + e.what()).c_str());
		}

	});
	return QApplication::exec();
}