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
#include <qtimer.h>
#include <qcombobox.h>
#include "include/MainWindow.h"
#include "include/PdfWriter.h"


int runManual(int argc, char **argv) {

	MainWindow* window = MainWindow::getMainWindow();
	QBoxLayout* windowLayout = new QBoxLayout(QBoxLayout::TopToBottom);

	QLabel* description = new QLabel("Program to generate punch press programs from lay files.\nPlease Select the directory containing the lay files.", window);
	QComboBox* pathEdit = new QComboBox(window);
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

	QPushButton* completeButton = new QPushButton("Start", window);
	completeButton->setCheckable(true);
	windowLayout->addWidget(description);
	windowLayout->addWidget(pathWidget);
	windowLayout->addWidget(outputWidget);
	windowLayout->addWidget(completeButton);

	window->setLayout(windowLayout);

	QObject::connect(pathButton, &QPushButton::clicked, [pathEdit]() {pathEdit->clear(); pathEdit->addItems(QFileDialog::getOpenFileNames()); });
	QObject::connect(outputButton, &QPushButton::clicked, [outputEdit]() {outputEdit->setText(QFileDialog::getExistingDirectory()); });
	window->show();
	QObject::connect(completeButton, &QPushButton::clicked, [pathEdit, outputEdit, window, completeButton]() mutable {
		std::filesystem::path output(outputEdit->text().toStdString());
		PdfWriter::setOutput(output);

					for (int i = 0; i < pathEdit->count(); i++) {
						std::filesystem::path path(pathEdit->itemText(i).toStdString());
						if (path.has_extension() && path.extension() == ".lay") {
							PdfWriter::makePdf(path.c_str(), window);
							qApp->processEvents();
						}
					}
					QMessageBox::information(window, "Complete", "All files have been completed");
					 });

	return QApplication::exec();
}

