// auto_delete : lightweight objects to manage object deletion when leaving scope

#ifndef auto_delete_H
#define auto_delete_H 1

template <class T>
class auto_delete
{
public:
	explicit auto_delete(T* p) { mObj = p; }
	~auto_delete() { delete mObj; }

private:
	auto_delete(const auto_delete<T>&);
	auto_delete<T>& operator=(const auto_delete<T>&);
	T* mObj;
};

template <class T>
class auto_array_delete
{
public:
	explicit auto_array_delete(T* p) { mObj = p; }
	~auto_array_delete() { delete [] mObj; }

private:
	auto_array_delete(const auto_array_delete<T>&);
	auto_array_delete<T>& operator=(const auto_array_delete<T>&);
	T* mObj;
};

#endif
