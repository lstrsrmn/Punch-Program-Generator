#include <iostream>
#include <QPushButton.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include "include/PdfWriter.h"

#if defined(unix)
typedef char* LPTSTR;
typedef char TCHAR;
#else
#include "windows.h"
#include <tchar.h>
#endif

static std::map<std::filesystem::path, std::filesystem::file_time_type> dirData;

#if (!unix)
void RefreshDirectory(LPTSTR lpDir) {
	std::filesystem::path path(lpDir);

	for (const std::filesystem::directory_entry& p : std::filesystem::directory_iterator(path)) {
		if (!p.path().has_extension())
			continue;
		if (p.path().extension() != ".lay")
			continue;
		if (dirData.contains(p) && dirData[p] != p.last_write_time()) {
			PdfWriter::makePdf(p);
		}
		else if (!dirData.contains(p)) {
			PdfWriter::makePdf(p);
		}
		else {
			dirData[p] = p.last_write_time();
		}
	}
	std::cout << std::endl;
}

void watch_directory(LPTSTR path, HANDLE exit_watch) {

	for (std::filesystem::directory_entry p : std::filesystem::directory_iterator(path)) {
		dirData[p] = p.last_write_time();
	}

	HANDLE dirChanged;
	dirChanged = FindFirstChangeNotification(path, false, FILE_NOTIFY_CHANGE_LAST_WRITE || FILE_NOTIFY_CHANGE_FILE_NAME);
	if (dirChanged == INVALID_HANDLE_VALUE) {
		std::cout << "ERROR: FindFirstChangeNotification function failed." << std::endl;
		ExitProcess(GetLastError());
	}

	if (dirChanged == NULL) {
		std::cout << "ERROR: Unexpected NULL from FindFirstChangeNotification." << std::endl;
		ExitProcess(GetLastError());
	}
	HANDLE watched[2] = {dirChanged, exit_watch};
	while (true) {
		std::cout << "Waiting for notification at " << path << std::endl;
		DWORD dwWaitStatus = WaitForMultipleObjects(1, watched, false, INFINITE);
		switch (dwWaitStatus) {
			case WAIT_OBJECT_0 :
			std::cout << "Notified at " << std::ctime(new std::time_t(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))) << std::endl;
			std::this_thread::sleep_for(std::chrono::minutes(2));
			std::cout << "Starting at " << std::ctime(new std::time_t(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))) << std::endl;
			RefreshDirectory(path); 
             if ( FindNextChangeNotification(dirChanged) == FALSE )
             {
               printf("\n ERROR: FindNextChangeNotification function failed.\n");
               ExitProcess(GetLastError()); 
			 }
             break;
			case WAIT_OBJECT_0 + 1 :
				return;
		}
	}
}
#endif