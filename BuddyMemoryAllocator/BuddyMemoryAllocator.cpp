#include <cstdlib>
#include <iostream>
#include <list>
#include <fstream>
#include <string>
#include <exception>

double max(double a, double b)
{
	return a > b ? a : b;
}

struct BlockDescriptor
{
	BlockDescriptor* prevSameOrder;
	BlockDescriptor* nextSameOrder;
	unsigned short order;
	bool isFree;
};

template <unsigned short MaxOrder>
class MemoryManager
{
private:
	void* memoryStart;
	int maxUsableMemorySize;
	int sizeAvailable;
	bool initialized;
	int minOrderBlockSize;
	BlockDescriptor* orderBlockListStart[MaxOrder + 1];

	int getBlockSize(const int order) const
	{
		return minOrderBlockSize << order;
	}

	BlockDescriptor* getBlockBuddy(BlockDescriptor* block) const
	{
		return (BlockDescriptor*)(size_t(memoryStart) + (getBlockOffset(block) ^ getBlockSize(block->order)));
	}

	BlockDescriptor* tryMergeFreeBuddies(BlockDescriptor* block)
	{
		if (block->order == MaxOrder)
			return nullptr;

		auto buddy = getBlockBuddy(block);
		if (!canMergeBuddies(block, buddy)) return nullptr;

		removeFreeBlock(block);
		removeFreeBlock(buddy);

		const auto mergedBlock = block < buddy ? block : buddy;
		mergedBlock->order++;
		insertFreeBlock(mergedBlock);
		return mergedBlock;
	}

	int getBlockOffset(BlockDescriptor* block) const
	{
		return size_t(block) - size_t(memoryStart);
	}

	static bool canMergeBuddies(BlockDescriptor* block1, BlockDescriptor* block2)
	{
		return block1->isFree && block2->isFree && block1->order == block2->order;
	}

	bool canMergeWithBuddy(BlockDescriptor* block) const
	{
		return canMergeBuddies(block, getBlockBuddy(block));;
	}

	void insertFreeBlock(BlockDescriptor* block)
	{
		BlockDescriptor* prev = nullptr;
		BlockDescriptor* current = orderBlockListStart[block->order];
		if (current == nullptr || block < current)
		{
			orderBlockListStart[block->order] = block;
			block->prevSameOrder = nullptr;
			block->nextSameOrder = current;
			return;
		}

		while (current < block && current != nullptr)
		{
			prev = current;
			current = current->nextSameOrder;
		}

		if (current == block)
			return;

		block->prevSameOrder = prev;
		block->nextSameOrder = current;

		prev->nextSameOrder = block;
		if (current != nullptr)
			current->prevSameOrder = block;
	}

	void removeFreeBlock(BlockDescriptor* block)
	{
		if (block->nextSameOrder != nullptr)
			block->nextSameOrder->prevSameOrder = block->prevSameOrder;
		if (block->prevSameOrder != nullptr)
			block->prevSameOrder->nextSameOrder = block->nextSameOrder;

		if (orderBlockListStart[block->order] == block)
			orderBlockListStart[block->order] = block->nextSameOrder;

		block->nextSameOrder = block->prevSameOrder = nullptr;
	}

	/**
	 * \brief Делит блоки на два двойника вдвое меньшего размера
	 * \param block - блок для разделения, далее по этому адресу будет распологаться левый двойник 
	 * \return Адрес правого двойника
	 */
	BlockDescriptor* splitToBuddies(BlockDescriptor* block)
	{
		if (block->order == 0) return block;

		removeFreeBlock(block);

		block->order--;

		insertFreeBlock(block);

		const auto newBlock = (BlockDescriptor*)(size_t(block) + getBlockSize(block->order));
		newBlock->order = block->order;
		newBlock->isFree = true;
		newBlock->nextSameOrder = block->nextSameOrder;
		newBlock->prevSameOrder = block;
		block->nextSameOrder = newBlock;

		sizeAvailable -= sizeof(BlockDescriptor);

		return newBlock;
	}

public:
	bool isInitialized() const
	{
		return initialized;
	}

	explicit MemoryManager(const int size)
	{
		memoryStart = std::malloc(size);
		if (memoryStart == nullptr)
		{
			initialized = false;
			return;
		}

		const int flooredLog2 = int(std::floor(log2(size)));
		if (flooredLog2 < MaxOrder)
			throw std::runtime_error("MaxOrder " + std::to_string(MaxOrder) + " is too big for size " + std::to_string(size));

		minOrderBlockSize = 1 << (flooredLog2 - MaxOrder);

		if (minOrderBlockSize <= sizeof(BlockDescriptor))
			throw std::runtime_error("MaxOrder " + std::to_string(MaxOrder) + " is too big for size " + std::to_string(size));

		sizeAvailable = maxUsableMemorySize = getBlockSize(MaxOrder);

		initialized = true;
		for (int i = 0; i <= MaxOrder; ++i)
			orderBlockListStart[i] = nullptr;

		const auto firstBlock = (BlockDescriptor*)memoryStart;
		firstBlock->prevSameOrder = nullptr;
		firstBlock->nextSameOrder = nullptr;
		firstBlock->order = MaxOrder;
		firstBlock->isFree = true;
		insertFreeBlock(firstBlock);

		sizeAvailable -= sizeof(BlockDescriptor);
	}

	int allocate(const int size)
	{
		if (!isInitialized() && sizeAvailable < size)
			return -1;

		const int sizeRequired = size + sizeof(BlockDescriptor);
		const int orderRequired = int(std::ceil(log2(max(1.0, (double)sizeRequired / minOrderBlockSize))));
		BlockDescriptor* selectedBlock = nullptr;

		for (auto order = orderRequired; order <= MaxOrder; ++order)
		{
			if (orderBlockListStart[order] != nullptr)
			{
				selectedBlock = orderBlockListStart[order];
				while (order > orderRequired)
				{
					splitToBuddies(selectedBlock);
					--order;
				}
				break;
			}
		}

		if (selectedBlock == nullptr)
			return -1;

		removeFreeBlock(selectedBlock);
		selectedBlock->isFree = false;
		return int(getBlockOffset(selectedBlock) + sizeof(BlockDescriptor));
	}

	bool free(const int pointer)
	{
		if (pointer < sizeof(BlockDescriptor))
			return false;

		auto block = (BlockDescriptor*)(size_t(memoryStart) + pointer - sizeof(BlockDescriptor));
		if (block->isFree)
			return false;
		block->isFree = true;

		insertFreeBlock(block);
		auto freedSize = getBlockSize(block->order);

		while (canMergeWithBuddy(block))
		{
			block = tryMergeFreeBuddies(block);
			freedSize += sizeof(BlockDescriptor);
		}

		sizeAvailable += freedSize;
		return true;
	}


	~MemoryManager()
	{
		std::free(memoryStart);
	}
};


struct TestCase
{
	bool is_allocating;
	int param;
};

std::string readInput(_In_ const std::string& testPath)
{
	auto sInputFile = std::ifstream(testPath);
	if (!sInputFile.is_open())
	{
		std::cerr << "Error: can't open test file." << std::endl;
		return "";
	}

	auto strRaw = std::string(
		std::istreambuf_iterator<char>(sInputFile),
		std::istreambuf_iterator<char>()
	);

	sInputFile.close();
	return strRaw;
}

std::list<std::string> splitString(_In_ const std::string& str, _In_ const std::string& sep)
{
	auto strCopy = std::string(str);

	std::list<std::string> result;

	size_t sepIndex = std::string::npos;
	while ((sepIndex = strCopy.find(sep)) != std::string::npos)
	{
		result.push_back(strCopy.substr(0, sepIndex));
		strCopy = strCopy.substr(sepIndex + sep.length());
	}
	result.push_back(strCopy.substr(0, sepIndex));

	return result;
}

void doTests(_In_ const std::string& testPath)
{
	const auto rawTests = readInput(testPath);
	auto lTests = splitString(rawTests, "\n");

	auto strMemSize = lTests.front();
	lTests.pop_front();

	auto manager = MemoryManager<4>(atoi(strMemSize.c_str()));

	auto testNumber = 0;
	for (const auto iter : lTests)
	{
		auto lTestData = splitString(iter, "|");
		TestCase current_test;
		if ((&lTestData.front())[0][0] == '+')
			current_test.is_allocating = true;
		else
			current_test.is_allocating = false;

		current_test.param = atoi(lTestData.front().substr(1).c_str());
		const int expectedAnswer = atoi(lTestData.back().c_str());

		const int acutalAnswer = current_test.is_allocating
			                         ? manager.allocate(current_test.param)
			                         : manager.free(current_test.param)
			                         ? 1
			                         : 0;

		std::cout << "Test #" << testNumber << " :";
		if (acutalAnswer == expectedAnswer)
		{
			std::cout << "OK" << std::endl;
		}
		else
		{
			std::cout << "Failed with";
			std::cout << " Expected: " << expectedAnswer;
			std::cout << " Actual: " << acutalAnswer << std::endl;
		}
		testNumber++;
	}
}

int main()
{
	doTests("test.txt");
	return 0;
}
