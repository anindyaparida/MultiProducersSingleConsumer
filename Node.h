/*
	Queue node for link list of data / buffer
*/

#define NODE_SIZE 4096

template <class T>
class Node {
    public:
        Data<T> nodeData[DATA_SIZE];

		std::atomic<Node*> next;
		Node* prev;
		unsigned int head;
		unsigned int queuePosition;

		Node()
			: next(NULL), prev(NULL), head(0), queuePosition(0)
		{	
		}
		Node(unsigned int bufferSizez)
			:  next(NULL), prev(NULL), head(0), queuePosition(1)
		{
			memset(nodeData, 0, DATA_SIZE *sizeof(Data<T>) );
		}
		Node(unsigned int bufferSizez , unsigned int positionInQueue , Node* prev)
			: next(NULL), prev(prev), head(0), queuePosition(positionInQueue)
		{
			memset(nodeData, 0, DATA_SIZE * sizeof(Data<T>));
		}
	};