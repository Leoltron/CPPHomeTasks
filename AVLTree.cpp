#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <list>
#include <valarray>

template <typename TK, typename TV>
struct Node
{
	TK key;
	TV value;
	Node<TK, TV>* rightChild;
	Node<TK, TV>* leftChild;
	Node<TK, TV>* parent;

	//Node() = default;

	Node(const TK& key, const TV& value, Node<TK, TV>* right_child, Node<TK, TV>* left_child, Node<TK, TV>* parent)
		: key(key),
		  value(value),
		  rightChild(right_child),
		  leftChild(left_child),
		  parent(parent)
	{
	}

	int getTreeSize()
	{
		int size = 1;
		if (leftChild != nullptr)
			size += leftChild->getTreeSize();
		if (rightChild != nullptr)
			size += rightChild->getTreeSize();

		return size;
	}

	Node<TK, TV>* getMaxRoot()
	{
		return parent == nullptr ? this : parent->getMaxRoot();
	}

	Node<TK, TV>* farthestLeftChild()
	{
		return leftChild == nullptr ? this : leftChild->farthestLeftChild();
	}

	Node<TK, TV>* smallRightRotation()
	{
		Node<TK, TV>* left = leftChild;
		leftChild = left->rightChild;
		left->rightChild = this;
		left->parent = parent;
		parent = left;
		if (left->parent != nullptr)
		{
			if (left->key > left->parent->key)
				left->parent->rightChild = left;
			else
				left->parent->leftChild = left;
		}
		return left;
	}

	Node<TK, TV>* smallLeftRotation()
	{
		Node<TK, TV>* right = rightChild;
		rightChild = right->leftChild;
		right->parent = parent;
		parent = right;
		right->leftChild = this;
		if (right->parent != nullptr)
		{
			if (right->key > right->parent->key)
				right->parent->rightChild = right;
			else
				right->parent->leftChild = right;
		}
		return right;
	}
};

template <typename TK, typename TV>
int getNodeHeight(Node<TK, TV>* node, const int parentLevel)
{
	if (node == nullptr)
		return parentLevel;

	return std::max(
		getNodeHeight(node->leftChild, parentLevel + 1),
		getNodeHeight(node->rightChild, parentLevel + 1)
	);
}


template <typename TK, typename TV>
int getNodeHeight(Node<TK, TV>* node)
{
	return getNodeHeight(node, 0);
}


template <typename TK, typename TV>
bool isBinaryTreeNodeValid(Node<TK, TV>* n)
{
	if (n == nullptr)
		return true;
	if (n->leftChild != nullptr)
	{
		if (n->key < n->leftChild->key)
			return false;

		if (!isBinaryTreeNodeValid(n->leftChild))
			return false;
	}

	if (n->rightChild != nullptr)
	{
		if (n->key > n->rightChild->key)
			return false;

		if (!isBinaryTreeNodeValid(n->rightChild))
			return false;
	}
	return true;
}


template <typename TK, typename TV>
int getBalanceFactor(Node<TK, TV>* node)
{
	return getNodeHeight(node->rightChild) - getNodeHeight(node->leftChild);
}

template <typename TK, typename TV>
bool isNodeBalanced(Node<TK, TV>* node)
{
	return node == nullptr ||
	((node->leftChild == nullptr || isNodeBalanced(node->leftChild)) &&
		(node->rightChild == nullptr || isNodeBalanced(node->rightChild)) &&
		std::abs(getBalanceFactor(node)) < 2);
}


template <typename TK, typename TV>
Node<TK, TV>* rebalanceTree(Node<TK, TV>* node)
{
	if (getBalanceFactor(node) == 2)
	{
		if (getBalanceFactor(node->rightChild) < 0)
			node->rightChild = node->rightChild->smallRightRotation();
		return node->smallLeftRotation();
	}
	else if (getBalanceFactor(node) == -2)
	{
		if (getBalanceFactor(node->leftChild) > 0)
			node->leftChild = node->leftChild->smallLeftRotation();
		return node->smallRightRotation();
	}
	return node;
}

template <typename TK, typename TV,
          // ReSharper disable once CppInconsistentNaming
          class _TKC = std::less<TK>>
class BST
{
private:
	Node<TK, TV>* root = nullptr;

	Node<TK, TV>* searchNode(TK key)
	{
		Node<TK, TV>* current = root;

		while (current != nullptr)
		{
			if (key == current->key)
				return current;

			if (key < current->key)
				current = current->leftChild;
			else
				current = current->rightChild;
		}
		return nullptr;
	}

public:

	Node<TK, TV>* getRoot() const
	{
		return root;
	}

	bool add(TK key, TV value)
	{
		Node<TK, TV>* prev = root;
		Node<TK, TV>* current = root;

		std::list<Node<TK, TV>*> nodes;

		while (current != nullptr)
		{
			nodes.push_back(current);
			if (key == current->key)
				return false;

			prev = current;
			if (key < current->key)
				current = current->leftChild;
			else
				current = current->rightChild;
		}

		Node<TK, TV>* newNode = new Node<TK, TV>(key, value, nullptr, nullptr, prev);

		if (prev == nullptr)
			root = newNode;
		else
		{
			if (key < prev->key)
				prev->leftChild = newNode;
			else
				prev->rightChild = newNode;
		}

		for (auto node : nodes)
			rebalanceTree(node);

		root = root->getMaxRoot();
		return true;
	}

	bool tryGet(_In_ TK key, _Out_ TV& value)
	{
		auto searchResult = searchNode(key);

		if (searchResult == nullptr)
			return false;

		value = searchResult->value;
		return true;
	}

	bool remove(TK key)
	{
		Node<TK, TV>* searchResult = searchNode(key);
		if (searchResult == nullptr)
			return false;

		Node<TK, TV>* parent = searchResult->parent;

		std::list<Node<TK, TV>*> nodes;
		Node<TK, TV>* current = parent;
		while (current != nullptr)
		{
			nodes.push_front(current);
			current = current->parent;
		}

		const bool isLeft = parent != nullptr && searchResult->key < parent->key;
		if (searchResult->leftChild == nullptr && searchResult->rightChild == nullptr)
		{
			if (parent != nullptr)
			{
				if (isLeft)
					parent->leftChild = nullptr;
				else
					parent->rightChild = nullptr;

				for (auto node : nodes)
					rebalanceTree(node);
				root = root->getMaxRoot();
			}
			else
			{
				root = nullptr;
			}

			delete searchResult;
			return true;
		}

		Node<TK, TV>* singleChild = nullptr;

		if (searchResult->leftChild != nullptr && searchResult->rightChild == nullptr)
			singleChild = searchResult->leftChild;
		if (searchResult->rightChild != nullptr && searchResult->leftChild == nullptr)
			singleChild = searchResult->rightChild;

		if (singleChild != nullptr)
		{
			if (parent != nullptr)
			{
				if (isLeft)
				{
					parent->leftChild = singleChild;
				}
				else
					parent->rightChild = singleChild;

				for (auto node : nodes)
					rebalanceTree(node);
				root = root->getMaxRoot();
			}
			else
			{
				root = singleChild;
			}

			singleChild->parent = parent;
			delete searchResult;
			return true;
		}

		Node<TK, TV>* minRightNode = searchResult->rightChild->farthestLeftChild();

		TK minRightNodeKey = minRightNode->key;
		TV minRightNodeValue = minRightNode->value;

		remove(minRightNodeKey);

		searchResult->key = minRightNodeKey;
		searchResult->value = minRightNodeValue;

		rebalanceTree(searchResult);
		for (auto node : nodes)
			rebalanceTree(node);
		root = root->getMaxRoot();

		return true;
	}

	int getTreeHeight()
	{
		return getNodeHeight(root);
	}

	bool isBinaryTreeValid()
	{
		return isBinaryTreeNodeValid(root);
	}

	int getTreeSize()
	{
		return root == nullptr ? 0 : root->getTreeSize();
	}

	bool isTreeBalanced()
	{
		return isNodeBalanced(root);
	}
};

struct TestCase
{
	int key;
	std::string command;
	int value;
	bool expected;


	TestCase(int key, const std::string& command, int value, bool expected)
		: key(key),
		  command(command),
		  value(value),
		  expected(expected)
	{
	}
};

std::string Add = "add";
std::string Remove = "remove";
std::string Find = "find";

std::vector<TestCase> TestCases = {
	//Binary tree tests
	TestCase(4, Add, 12, true),
	TestCase(4, Add, 12, false),
	TestCase(4, Remove, 12, true),
	TestCase(4, Remove, 12, false),
	TestCase(4, Add, 12, true),
	TestCase(1, Add, 13, true),
	TestCase(6, Add, 14, true),
	TestCase(5, Add, 15, true),
	TestCase(7, Add, 16, true),
	TestCase(4, Remove, 12, true),
	TestCase(4, Find, 12, false),
	TestCase(5, Find, 15, true),
	TestCase(1, Find, 13, true),
	TestCase(7, Find, 16, true),
	//AVL balance tests
	TestCase(8, Add, 17, true),
	TestCase(9, Add, 18, true),
	TestCase(10, Add, 19, true),
	TestCase(11, Add, 19, true),
	TestCase(12, Add, 20, true),
	TestCase(13, Add, 21, true),
	TestCase(4, Add, 12, true),
	TestCase(6, Remove, 14, true),
	TestCase(10, Remove, 19, true),

};

int main()
{
	BST<int, int> bst;
	int i = 0;
	bool result = false;
	int val;
	for (auto case_ : TestCases)
	{
		++i;
		switch (case_.command[0])
		{
		case 'a':
			result = bst.add(case_.key, case_.value);
			break;
		case 'f':
			result = bst.tryGet(case_.key, val);
			if (result && case_.expected && val != case_.value)
			{
				result = false;
				std::cout << "Test #" << i << ": TryGet: invalid value: " << val << " (expected: " << case_.value << ")" << std::
					endl;
			}
			break;
		case 'r':
			result = bst.remove(case_.key);
			break;
		default:
			std::cout << "Test #" << i << ": ignored: invalid command " << case_.command << std::endl;
		}
		std::cout << "Test #" << i << ": ";
		if (result == case_.expected)
			std::cout << "OK";
		else
			std::cout << "Error: expected " << case_.expected << std::endl;
		std::cout << std::endl;

		if (!bst.isBinaryTreeValid())
			std::cout << "\tInvalid binary tree" << std::endl;
		else
			std::cout << "\tBinary tree rules: OK" << std::endl;

		if (!bst.isTreeBalanced())
			std::cout << "\tTree is not balanced enough (size: " << bst.getTreeSize() << ", height: " << bst.getTreeHeight() <<
				")" << std::endl;
		else
			std::cout << "\tBalance: OK" << std::endl;
	}
	return 0;
}
