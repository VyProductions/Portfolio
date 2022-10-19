#include <string>

template <class InputIt, class OutputIt>
static void copy(InputIt first, InputIt last, OutputIt d_first);

template <class Iterator, class T>
static Iterator find(Iterator first, Iterator last, const T& value);

struct alarm_t {
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int second;
    bool valid;
    std::size_t id;
    bool hit;
    std::string desc;
};

template <class T>
class DynamicArray {
public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = value_type*;
    using const_iterator = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

    DynamicArray(size_type count = 0) : used(0), CAPACITY(count) {
        data = new value_type[CAPACITY];
    }

    DynamicArray(const DynamicArray& other) {
        used = other.used;
        CAPACITY = other.CAPACITY;
        data = !CAPACITY ? nullptr : new value_type[CAPACITY];

        ::copy(other.begin(), other.end(), begin());
    }

    ~DynamicArray() {
        clear();
    }

    void clear() {
        delete [] data;
        data = nullptr;
        used = 0;
        CAPACITY = 0;
    }

    size_type size() { return used; }
    size_type capacity() { return CAPACITY; }

    iterator begin() { return data; }
    const_iterator begin() const { return data; }

    iterator end() { return data + used; }
    const_iterator end() const { return data + used; }

    bool empty() { return !used; }

    reference first() {
        if (empty()) {
            throw std::logic_error("Cannot reference first of empty array.");
        }
        return *data;
    }

    const_reference first() const {
        if (empty()) {
            throw std::logic_error("Cannot reference first of empty array.");
        }
        return *data;
    }

    reference last() {
        if (empty()) {
            throw std::logic_error("Cannot reference last of empty array.");
        }
        return *(data + used - 1);
    }

    const_reference last() const {
        if (empty()) {
            throw std::logic_error("Cannot reference last of empty array.");
        }
        return *(data + used - 1);
    }

    reference at(size_type pos) {
        if (pos >= used) {
            throw std::logic_error("Access out of range.");
        }
        return *(data + pos);
    }

    void insert(value_type value, iterator pos = begin()) {
        if (pos < begin() || pos > end()) {
            throw std::logic_error("Insert out of range.");
        }

        if (used == CAPACITY) {
            CAPACITY += 8;
            value_type* tmp = data;
            data = new value_type[CAPACITY];
            ::copy(tmp, tmp + used, data);
            delete [] tmp;
        }

        iterator q = pos;

        while (pos != end()) {
            *(pos + 1) = *pos;
        }

        *q = value;
        ++used;
    }

    void push_back(value_type value) {
        if (used == CAPACITY) {
            CAPACITY += 8;
            value_type* tmp = data;
            data = new value_type[CAPACITY];
            ::copy(tmp, tmp + used, data);
            delete [] tmp;
        }

        data[used++] = value;
    }

    value_type pop_back() {
        if (empty()) {
            throw std::logic_error("Cannot pop from empty array.");
        }

        return data[used--];
    }

    iterator erase(iterator pos = begin()) {
        if (pos < begin() || pos > end()) {
            throw std::logic_error("Erase out of range.");
        }

        while (pos != end()) {
            *pos = *(pos + 1);
        }

        --used;
    }
private:
    value_type* data;
    size_type used;
    size_type CAPACITY;
};

template <class InputIt, class OutputIt>
static void copy(InputIt first, InputIt last, OutputIt d_first) {
    while (first != last) {
        *d_first++ = *first++;
    }
}

template <class Iterator, class T>
static Iterator find(Iterator first, Iterator last, T value) {
    while (*first != value && first != last) {
        ++first;
    }

    return first;
}