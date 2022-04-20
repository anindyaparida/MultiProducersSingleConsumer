/*
Main Queue data structure where we have the enqueue and dequeue operation
*/

#include <atomic>
#include <iostream>
#include <string.h>
#include <thread>
#include <stdlib.h>

#include "Data.h"
#include "Node.h"
#include "Utility.h"

using std::cout;
using std::endl;

template <class T>
class Queue {

private:
	Node<T>* queueHead;
	std::atomic<Node<T>*> queueTail;
	unsigned int nodeSize;
	std::atomic<uint_fast64_t> insertPosition;

public:
	Queue(unsigned int size) :
		nodeSize(DATA_SIZE), queueTail(NULL), insertPosition(0)
	{
		void* node = allocateMem(NODE_SIZE, sizeof(Node<T>));
		queueHead = new(node)Node<T>(nodeSize);
		queueTail = queueHead;
	}
	Queue() :
		nodeSize(DATA_SIZE), queueTail(NULL), insertPosition(0)
	{
		void* node = allocateMem(NODE_SIZE, sizeof(Node<T>));
		queueHead = new(node)Node<T>(nodeSize);
		queueTail = queueHead;
	}

	~Queue()
	{
		while (queueHead->next.load(std::memory_order_acquire) != NULL)
		{
			Node<T>* next = queueHead->next.load(std::memory_order_acquire);
			freeMem(queueHead);
			queueHead = next;
		}
		freeMem(queueHead);
	}

	bool dequeue(T& value)
	{
		while (true)
		{
			Node<T>* tailNode = queueTail.load(std::memory_order_seq_cst);

			unsigned int remQueueSize = nodeSize*(tailNode->queuePosition - 1);
			if ((queueHead == queueTail.load(std::memory_order_acquire)) && (queueHead->head == (insertPosition.load(std::memory_order_acquire) - remQueueSize) ))  // the queue is empty
			{
				return false;
			}
			else if (queueHead->head < nodeSize) // there is elements in the current array.
			{
				Data<T>* dataValue = &(queueHead->nodeData[queueHead->head]);
				if (dataValue->is_set.load(std::memory_order_acquire) == 2) // Already processed
				{
					queueHead->head++;
					continue;
				}

				Node<T>* nodeHasData = queueHead;
				unsigned int dataNodeHead = nodeHasData->head;

				bool isNewNode = false , nodeProcessed=true;

				while (dataValue->is_set.load(std::memory_order_acquire) == 0) // data not available yet then next data
				{				
					if (dataNodeHead < nodeSize) // Node has further data
					{
						Data<T>* nextDataValue = &(nodeHasData->nodeData[dataNodeHead++]); // fetch next data point

						if (nextDataValue->is_set.load(std::memory_order_acquire) == 1 && dataValue->is_set.load(std::memory_order_acquire) == 0) // the data is valid and the element in the head is still in insert process 
						{
							Node<T>* scanQueue = queueHead;
							for (unsigned int scanHead = scanQueue->head; (scanQueue != nodeHasData || scanHead < (dataNodeHead-1) && dataValue->is_set.load(std::memory_order_acquire) == 0); scanHead++)
							{
								if (scanHead >= nodeSize)   // end of node , next node
								{
									scanQueue = scanQueue->next.load(std::memory_order_acquire);
									scanHead = scanQueue->head;
									continue;
								}

								Data<T>* scanDataValue = &(scanQueue->nodeData[scanHead]);
								if (scanDataValue->is_set.load(std::memory_order_acquire) == 1) // set data found
								{
									dataNodeHead = scanHead;
									nodeHasData = scanQueue;
									nextDataValue = scanDataValue;
									
									scanQueue = queueHead;
									scanHead = scanQueue->head;
								}							
							}

							if (dataValue->is_set.load(std::memory_order_acquire) == 1)
							{
								break;
							}
							
							value = nextDataValue->data;
							nextDataValue->is_set.store(2, std::memory_order_release); //set processed
							if (isNewNode && (dataNodeHead-1) == nodeHasData->head) // new node then set the head to new node
							{
								nodeHasData->head++;
							}
							
							return true;

						}
					
						 if(nextDataValue->is_set.load(std::memory_order_acquire) == 0)
						{ 
							nodeProcessed = false ;
						}	
					}

					if (dataNodeHead >= nodeSize)
					{
						if (nodeProcessed && isNewNode)
						{
							if (nodeHasData == queueTail.load(std::memory_order_acquire)) return false;

							Node<T>* next = nodeHasData->next.load(std::memory_order_acquire);
							Node<T>* prev = nodeHasData->prev;
							if (next == NULL) return false;

							next->prev = prev;
							prev->next.store(next, std::memory_order_release);
							freeMem(nodeHasData);
							
							nodeHasData = next;
							dataNodeHead = nodeHasData->head;
							nodeProcessed = true;
							isNewNode = true;

						}
						else {
							Node<T>* next = nodeHasData->next.load(std::memory_order_acquire);
							if (next == NULL) return false;
							nodeHasData = next;
							dataNodeHead = nodeHasData->head;
							isNewNode = true;
							nodeProcessed = true;
						}
					}
				}
				
				if (dataValue->is_set.load(std::memory_order_acquire) == 1)//valid
				{
					queueHead->head++;
					value = dataValue->data;
					return true;
				}
				
			}

			if (queueHead->head >= nodeSize) // move to the next array and delete the prev array 
			{
				if (queueHead == queueTail.load(std::memory_order_acquire)) return false;

				Node<T>* next = queueHead->next.load(std::memory_order_acquire);

				if (next == NULL) return false;

				freeMem(queueHead);
				queueHead = next;
			}
		}
	}

	void enqueue(T const& value)
	{
		
		Node<T>* tailNode;
		unsigned int queueIndex = insertPosition.fetch_add(1, std::memory_order_seq_cst);
		bool isPreviousIndex = false;
		while (true)
		{
			tailNode = queueTail.load(std::memory_order_acquire); // load last node
			
			unsigned int remQueueSize = nodeSize*(tailNode->queuePosition-1); // the amount of items in the queue without the current node

			while (queueIndex < remQueueSize) // the insert position is back in the queue
			{
				isPreviousIndex = true;
				tailNode = tailNode->prev;
				remQueueSize -= nodeSize;
			}

			unsigned int queueSize = nodeSize + remQueueSize; // the size of items in the queue 

			if (remQueueSize <= queueIndex && queueIndex < queueSize) // we are in the right node 
			{
				int insertIndex = queueIndex - remQueueSize;

				Data<T>* dataValue = &(tailNode->nodeData[insertIndex]);
					dataValue->data = value;
					dataValue->is_set.store(1, std::memory_order_relaxed);

					if (insertIndex == 1 && !isPreviousIndex) // allocating a new node and adding it to the queue
					{
						void* newNode = allocateMem(NODE_SIZE, sizeof(Node<T>));
						Node<T>* newArr =new(newNode)Node<T>(nodeSize, tailNode->queuePosition + 1, tailNode);
						Node<T>* Nullptr = NULL;

						if (!(tailNode->next).compare_exchange_strong(Nullptr, newArr))
						{
							freeMem(newArr);
						}
					}

					return;			
			}

			if (queueIndex >= queueSize) // the insert index in the ahaead node
			{
				Node<T>* next = (tailNode->next).load(std::memory_order_acquire);
				if (next == NULL)
				{
					void* newNode = allocateMem(NODE_SIZE, sizeof(Node<T>));

					Node<T>* newArr = new(newNode)Node<T>(nodeSize, tailNode->queuePosition + 1, tailNode);
					Node<T>* Nullptr = NULL;

						if ((tailNode->next).compare_exchange_strong(Nullptr, newArr))
						{
							queueTail.store(newArr, std::memory_order_release);
						}
						else
						{
							freeMem(newArr);
						}	
				}
				else 
				{
					queueTail.compare_exchange_strong(tailNode, next); // if it is not null move to the next node;
				}
			}
		}
	}
	
};
