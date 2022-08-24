#include <qapplication.h>
#include "include/PdfWriter.h"
#include <QPrinter.h>
#include <qprinterinfo.h>

int runApp();
int runManual(int, char**);

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "--debug") == 0) {
				PdfWriter::setDebug(true);
			} else if (strcmp(argv[i], "--manual") == 0) {
				return runManual(argc, argv);
			}
		}
	}
	return runApp();
}