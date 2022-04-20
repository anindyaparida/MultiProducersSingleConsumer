/*
	Data node for holding the code data in array
*/

#define DATA_SIZE 1620

template <class T>
class Data {
    public:
        T data;
		std::atomic<char> is_set ;
		Data() :data(), is_set(0)
		{
				
		}
		Data(T d) : data(d), is_set(0)
		{
				
		}

		Data(const Data &n)
		{
			data = n.data;
			is_set = n.is_set;
		}
};