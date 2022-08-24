#include <QWidget>

class MainWindow : public QWidget {
	Q_OBJECT
public:
	static MainWindow* getMainWindow() {
		if (window == nullptr)
			return new MainWindow();
		else {
			return window;
		}
	}

	void refresh() {
		this->update();
	}
private:
	MainWindow(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags()) : QWidget() {
	}
	static MainWindow* window;
};