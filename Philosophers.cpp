#define _CRT_SECURE_NO_WARNINGS

#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include <atomic>
#include <vector>

class Waiter
{
private:
	std::condition_variable conditionVariable;
	std::mutex mutex;
	std::atomic_uint placesLeftOnTable;

public:
	explicit Waiter(const unsigned int placesOnTable)
		: placesLeftOnTable(placesOnTable)
	{
	}

	void leaveTable()
	{
		std::unique_lock<std::mutex> lock(mutex);
		++placesLeftOnTable;
		conditionVariable.notify_one();
	}

	void enterTable()
	{
		std::unique_lock<std::mutex> lock(mutex);
		while (placesLeftOnTable == 0)
			conditionVariable.wait(lock);
		--placesLeftOnTable;
	}
};

class Fork
{
private:
	std::mutex isTaken;
public:
	void take() { isTaken.lock(); }

	void putAway() { isTaken.unlock(); }
};

class Philosopher
{
private:
	std::string name;

	static constexpr int thinkDelay = 50;
	static constexpr int eatDelay = 100;

	typedef std::chrono::high_resolution_clock time;

	int hungerTime = 0;
	int finishedMealsCount = 0;

	std::atomic_bool stopFlag = false;

	void doAction(const std::string& action, const int delayMillis) const
	{
		std::printf("%s: started %s...\n", name.c_str(), action.c_str());
		std::this_thread::sleep_for(std::chrono::milliseconds(delayMillis));
		std::printf("%s: stopped %s.\n", name.c_str(), action.c_str());
	}

	void think() const
	{
		doAction("thinking", thinkDelay);
	}

	void eat() const
	{
		doAction("eating", eatDelay);
	}

public:
	explicit Philosopher(const std::string& name)
		: name(name)
	{
	}

	void run(Fork* left, Fork* right, Waiter* waiter)
	{
		while (!stopFlag)
		{
			think();

			const auto hunger_start = time::now();

			waiter->enterTable();

			std::printf("%s: try take left fork\n", name.c_str());
			left->take();

			std::printf("%s: try take right fork\n", name.c_str());
			right->take();
			
			hungerTime += int(std::chrono::duration_cast<std::chrono::milliseconds>(time::now() - hunger_start).count());

			eat();
			finishedMealsCount++;

			right->putAway();
			std::printf("%s: put away right fork\n", name.c_str());
			
			left->putAway();
			std::printf("%s: put away left fork\n", name.c_str());
			
			waiter->leaveTable();
		}
	}

	void stop()
	{
		stopFlag = true;
	}

	void reportResults() const
	{
		std::printf("%s\n", name.c_str());
		std::printf("\tHunger time: %d\n", hungerTime);
		std::printf("\tMeales finished: %d\n", finishedMealsCount);
	}
};

const int phil_count = 5;

int main()
{
	setbuf(stdout, NULL);

	Fork* forks[phil_count];
	Waiter* waiter = new Waiter(phil_count-1);
	for (int i = 0; i < phil_count; i++)
		forks[i] = new Fork;

	Philosopher* philosophers[phil_count];
	std::vector<std::string> names = {
		"Kant", "Aristotle", "Nietzsche", "Socrates", "Diogenes"
	};

	for (int i = 0; i < phil_count; i++)
		philosophers[i] = new Philosopher("["+std::to_string(i)+"] "+names[i]);

	std::thread threads[phil_count];

	for (int i = 0; i < phil_count; i++)
		threads[i] = std::thread(&Philosopher::run, philosophers[i], forks[(i + 1) % phil_count], forks[i], waiter);


	std::this_thread::sleep_for(std::chrono::seconds(60));

	for (auto ph : philosophers)
		ph->stop();

	for (int i = 0; i < phil_count; i++)
		threads[i].join();

	for (auto ph : philosophers)
		ph->reportResults();

	for (int i = 0; i < phil_count; i++)
	{
		delete forks[i];
		delete philosophers[i];
	}
	delete waiter;

	return 0;
}
