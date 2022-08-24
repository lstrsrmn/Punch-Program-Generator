#include <filesystem>
#include <string>
#include <set>
#include <qpdfwriter.h>
#include <qstring.h>
#include <qpaintengine.h>
#include <iostream>
#include <QPrinterInfo.h>
#include "UniqueVector.h"

#if (!unix)
#include "Windows.h"
#endif

class PdfWriter {
private:
	class PressDrawing;
public:
	static void makePdf(const std::filesystem::path&, QWidget* = nullptr);
	static void setDebug(bool debug) {
		PdfWriter::debug = debug;
	}
	static void setOutput(const std::filesystem::path& path) {
		PdfWriter::output = path;
	}
	static void setManual(bool b) {
		PdfWriter::manual = b;
	}
#if (!unix)
	static LPTSTR convertStrTLPTSTR(const std::string& s) {
		LPTSTR o = new TCHAR[s.size() + 1];
		strcpy(o, s.c_str());
		return o;
	}
#endif
private:
	PdfWriter();

	static void drawPdf(const PressDrawing* p);
	static std::pair<std::string, std::string> split(std::string str, const char* del);
	static bool debug;
	static bool manual;
	static std::filesystem::path output;

	static bool printPdf(std::filesystem::path path);
	enum OverlapSide {
		NONE,
		BOTH,
		RIGHT
	};

	enum AperturePattern {
		ROUND,
		SQUARE,
		SLOTT,
		SLOTL,

		MISSING
	};
	static std::string aperture_name(const PdfWriter::AperturePattern& P) {
		switch (P) {
			case ROUND:
				return "ROUND";
			case SQUARE:
				return "SQUARE";
			case SLOTT:
				return "SLOTT";
			case SLOTL:
				return "SLOTL";
            default:
                return "";
		}
	}

	static std::string aperture_initials(const PdfWriter::AperturePattern& P) {
		switch (P) {
			case ROUND:
				return "DIA";
			case SQUARE:
				return "SQ";
			case SLOTT:
				return "SL";
			case SLOTL:
				return "SLR";
            default:
                    return "";
		}
	}

	class BadFileException : public std::exception {
	public:
		enum Errors {
			MISSING,
			CORRUPT,
			UNKNOWN
		};

		BadFileException(PdfWriter::BadFileException::Errors e, const std::filesystem::path& path) {
			this->path = path;
			this->e = e;
		}
		std::string str() const {
			switch (e) {
				case CORRUPT:
					return ("File corrupt at " + path.string());
				case MISSING:
					return ("File missing at " + path.string());
				case UNKNOWN:
					return ("Uknown issue with file " + path.string());
			}
			return "issue with exceptions";
		}
	private:
		std::filesystem::path path;
		Errors e;
	};

	class Coord {
		public:
		float x1;
		float x2;
		Coord() {}
		bool operator==(const Coord& other) const {
			return (this->x1 == other.x1 && this->x2 == other.x2);
		}
		struct CoordComp {
			bool operator() (const Coord& lhs, const Coord& rhs) const {
				if (lhs.x1 != rhs.x1)
					return lhs.x1 > rhs.x1;
				return lhs.x2 > rhs.x2;
			}
		};
	};

	class PressDrawing {
		friend class PdfWriter;
	public:
		PressDrawing(std::filesystem::path path);
		const std::string get_name() const {
			return this->name;
		}
	private:
		std::filesystem::path path;
		std::string name;
		std::string pattern;
		std::string drawing_no;
		std::string customer;
		std::string drawn_by;
		std::string date;
		std::string material;
		int thickness;
		std::string cutter_size;
		int length;
		int width;
		std::string bot_hook;
		std::string top_hook;
		int overlap;
		std::string understrips;
		int no_of_holes;
		double open_area;
		int rebate_width;
		int rebate_depth;
		PdfWriter::AperturePattern aperture_pattern;
		float size_x;
		float size_y;
		int no_of_cutters;
		int cutter_pitch;
		int shim_size;
		int press_strokes;
		UniqueVector<Coord> x_values;
		UniqueVector<Coord> y_values;
		float left_border;
		float right_border;
		float top_border;
		float bottom_border;
		int block_count;
		std::string notes[4];
		float bridge_x;
		float bridge_y;
		bool stagger_x;
		bool stagger_y;
		float space_x;
		float space_y;
		std::vector<std::vector<Coord>> points;
		int plate_width;
		PdfWriter::OverlapSide overlap_side;
		int tool_width;
		std::optional<int*> unpunched;
	};

};
