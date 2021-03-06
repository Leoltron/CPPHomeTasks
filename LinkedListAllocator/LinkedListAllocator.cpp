#include <cstdlib>
#include <iostream>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <list>

struct BlockDescriptor
{
	BlockDescriptor* prev;
	BlockDescriptor* next;
	int size;
	bool isFree;
};

class MemoryManager
{
private:
	void* memory;
	int allMemorySize;
	int sizeAvailable;
	bool initState;

public:
	explicit MemoryManager(int size);
	int allocate(int size);
	bool free(int pointer);
};

int MemoryManager::allocate(int size)
{
	BlockDescriptor* currentBlock = (BlockDescriptor*)memory;
	BlockDescriptor* selectedBlock = nullptr;

	while (currentBlock != nullptr)
	{
		if (currentBlock->isFree && currentBlock->size >= size)
		{
			selectedBlock = currentBlock;
			break;
		}
		currentBlock = currentBlock->next;
	}

	if (selectedBlock == nullptr)
		return -1;
	selectedBlock->isFree = false;
	const auto oldSize = selectedBlock->size;

	if (oldSize <= int(size + sizeof(BlockDescriptor)))
	{
		sizeAvailable -= oldSize;
		return int(size_t(selectedBlock) - size_t(memory) + sizeof(BlockDescriptor));
	}

	selectedBlock->size = size;
	const auto newBlock = (BlockDescriptor*)(size_t(selectedBlock) + sizeof(BlockDescriptor) + size);
	newBlock->size = oldSize - size - sizeof(BlockDescriptor);
	newBlock->next = selectedBlock->next;
	newBlock->prev = selectedBlock;
	newBlock->isFree = true;
	selectedBlock->next = newBlock;

	sizeAvailable -= size + sizeof(BlockDescriptor);

	return int(size_t(selectedBlock) - size_t(memory) + sizeof(BlockDescriptor));
}

BlockDescriptor* concatBlocks(BlockDescriptor* first, BlockDescriptor* second)
{
	if (first->next != second)
		return nullptr;

	if (first->next != nullptr)
	{
		first->next = second->next;
		first->size = int(first->size + second->size + sizeof(BlockDescriptor));
	}
	return first;
}

bool MemoryManager::free(const int pointer)
{
	if (pointer < sizeof(BlockDescriptor))
		return false;

	auto block = (BlockDescriptor*)(size_t(memory) + pointer - sizeof(BlockDescriptor));
	if (block->isFree)
		return false;

	auto freedSize = block->size;
	if (block->prev != nullptr && block->prev->isFree)
	{
		freedSize += sizeof(BlockDescriptor);
		block = concatBlocks(block->prev, block);
	}
	if (block->next != nullptr && block->next->isFree)
	{
		freedSize += sizeof(BlockDescriptor);
		block = concatBlocks(block, block->next);
	}

	block->isFree = true;
	sizeAvailable += freedSize;
	return true;
}

MemoryManager::MemoryManager(const int size)
{
	allMemorySize = size;
	memory = std::malloc(size);
	if (memory == nullptr)
	{
		initState = false;
		return;
	}

	initState = true;
	const auto firstBlock = (BlockDescriptor*)memory;
	firstBlock->prev = nullptr;
	firstBlock->next = nullptr;
	firstBlock->size = sizeAvailable = allMemorySize - sizeof(BlockDescriptor);
	firstBlock->isFree = true;
}


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
	auto rawTests = readInput(testPath);
	auto lTests = splitString(rawTests, "\n");

	std::string strMemSize = lTests.front();
	lTests.pop_front();

	auto manager = MemoryManager(atoi(strMemSize.c_str()));

	int testNumber = 0;
	for (auto iter : lTests)
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
