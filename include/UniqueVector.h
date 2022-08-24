#include <vector>

template<typename T>
class UniqueVector {
public:
	void push_back(const T& val) {
		for (T exists : data) {
			if (exists == val) {
				return;
			}
		}
		data.push_back(val);
	}

	void insert(const T& val) {
		for (T exists : data) {
			if (exists == val) {
				return;
			}
		}
		data.push_back(val);
	}

	T operator[](int index) const {
		return data[index];
	}

	int size() const {
		return data.size();
	}

	std::vector<T>::const_iterator begin() const {
		return data.begin();
	}

	std::vector<T>::const_iterator end() const {
		return data.end();
	}

	int find(T val) const {
		return data.find(val);
	}

private:
	std::vector<T> data;
};