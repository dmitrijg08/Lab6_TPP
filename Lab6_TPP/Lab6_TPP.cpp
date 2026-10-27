#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <barrier>
#include <condition_variable>
#include <queue>
#include <string>
#include <vector>
#include <future>
#include <chrono>
#include <iomanip>
#include <algorithm>

namespace Part1 {
    const int N_VARIANT = 8;
    const int ITERATIONS = N_VARIANT * 100000; // 800 000

    // 1.1. Без синхронізації (виникає Data Race)
    int counter_unsync = 0;
    void increment_unsync() {
        for (int i = 0; i < ITERATIONS; ++i) {
            ++counter_unsync;
        }
    }

    // 1.2. Синхронізація за допомогою std::mutex та std::lock_guard (RAII)
    int counter_mutex = 0;
    std::mutex counterMutex;
    void increment_mutex() {
        for (int i = 0; i < ITERATIONS; ++i) {
            std::lock_guard<std::mutex> lock(counterMutex);
            ++counter_mutex;
        }
    }

    // 1.3. Синхронізація за допомогою атомарної змінної std::atomic<int>
    std::atomic<int> counter_atomic{ 0 };
    void increment_atomic() {
        for (int i = 0; i < ITERATIONS; ++i) {
            ++counter_atomic;
        }
    }

    void run() {
        std::cout << "========================================================\n";
        std::cout << "PART 1: RACE CONDITION (Variant 8, ITERATIONS = " << ITERATIONS << ")\n";
        std::cout << "========================================================\n";
        const int expected = 2 * ITERATIONS;

        std::cout << "\n--- 1.1 Unsynchronized (10 runs) ---\n";
        int mismatch_count = 0;
        for (int run_idx = 1; run_idx <= 10; ++run_idx) {
            counter_unsync = 0;
            std::thread t1(increment_unsync);
            std::thread t2(increment_unsync);
            t1.join();
            t2.join();
            if (counter_unsync != expected) {
                ++mismatch_count;
            }
            std::cout << "Run " << std::setw(2) << run_idx
                << " | Expected: " << expected
                << " | Actual: " << std::setw(7) << counter_unsync
                << " | Lost: " << (expected - counter_unsync) << "\n";
        }
        std::cout << "Mismatches out of 10 runs: " << mismatch_count << "\n";

        std::cout << "\n--- 1.2 Synchronized with std::mutex (5 runs) ---\n";
        double total_mutex_ms = 0.0;
        for (int run_idx = 1; run_idx <= 5; ++run_idx) {
            counter_mutex = 0;
            auto start = std::chrono::steady_clock::now();
            std::thread t1(increment_mutex);
            std::thread t2(increment_mutex);
            t1.join();
            t2.join();
            auto end = std::chrono::steady_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            total_mutex_ms += ms;
            std::cout << "Run " << run_idx << " | Expected: " << expected
                << " | Actual: " << counter_mutex
                << " | Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
        }

        std::cout << "\n--- 1.3 Synchronized with std::atomic<int> (5 runs) ---\n";
        double total_atomic_ms = 0.0;
        for (int run_idx = 1; run_idx <= 5; ++run_idx) {
            counter_atomic.store(0);
            auto start = std::chrono::steady_clock::now();
            std::thread t1(increment_atomic);
            std::thread t2(increment_atomic);
            t1.join();
            t2.join();
            auto end = std::chrono::steady_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            total_atomic_ms += ms;
            std::cout << "Run " << run_idx << " | Expected: " << expected
                << " | Actual: " << counter_atomic.load()
                << " | Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
        }
        std::cout << "Avg time std::mutex : " << (total_mutex_ms / 5.0) << " ms\n";
        std::cout << "Avg time std::atomic: " << (total_atomic_ms / 5.0) << " ms\n\n";
    }
}

namespace Part2 {
    std::mutex mutexA;
    std::mutex mutexB;
    std::barrier<> syncPoint(2);

    // Варіант із Deadlock (для демонстрації зависання при запуску окремо)
    void task1_deadlock() {
        std::cout << "Thread 1: locking mutexA\n";
        std::lock_guard<std::mutex> lockA(mutexA);
        std::cout << "Thread 1: mutexA locked\n";
        syncPoint.arrive_and_wait();
        std::cout << "Thread 1: waiting for mutexB\n";
        std::lock_guard<std::mutex> lockB(mutexB);
        std::cout << "Thread 1: both mutexes locked\n";
    }

    void task2_deadlock() {
        std::cout << "Thread 2: locking mutexB\n";
        std::lock_guard<std::mutex> lockB(mutexB);
        std::cout << "Thread 2: mutexB locked\n";
        syncPoint.arrive_and_wait();
        std::cout << "Thread 2: waiting for mutexA\n";
        std::lock_guard<std::mutex> lockA(mutexA);
        std::cout << "Thread 2: both mutexes locked\n";
    }

    // Спосіб 1 усунення Deadlock: std::scoped_lock
    void task1_scoped() {
        syncPoint.arrive_and_wait();
        std::scoped_lock lock(mutexA, mutexB);
        std::cout << "Thread 1: both mutexes locked (std::scoped_lock)\n";
    }

    void task2_scoped() {
        syncPoint.arrive_and_wait();
        std::scoped_lock lock(mutexB, mutexA);
        std::cout << "Thread 2: both mutexes locked (std::scoped_lock)\n";
    }

    // Спосіб 2 усунення Deadlock: єдиний порядок захоплення (mutexA -> mutexB)
    void task1_ordered() {
        syncPoint.arrive_and_wait();
        std::lock_guard<std::mutex> lockA(mutexA);
        std::lock_guard<std::mutex> lockB(mutexB);
        std::cout << "Thread 1: both mutexes locked (ordered A -> B)\n";
    }

    void task2_ordered() {
        syncPoint.arrive_and_wait();
        std::lock_guard<std::mutex> lockA(mutexA);
        std::lock_guard<std::mutex> lockB(mutexB);
        std::cout << "Thread 2: both mutexes locked (ordered A -> B)\n";
    }

    void run() {
        std::cout << "========================================================\n";
        std::cout << "PART 2: DEADLOCK RESOLUTION\n";
        std::cout << "========================================================\n";

        std::cout << "\n--- 2.1 Fix with std::scoped_lock ---\n";
        {
            std::thread t1(task1_scoped);
            std::thread t2(task2_scoped);
            t1.join();
            t2.join();
            std::cout << "Program finished successfully (std::scoped_lock)\n";
        }

        std::cout << "\n--- 2.2 Fix with uniform lock ordering (mutexA -> mutexB) ---\n";
        {
            std::thread t1(task1_ordered);
            std::thread t2(task2_ordered);
            t1.join();
            t2.join();
            std::cout << "Program finished successfully (uniform lock ordering)\n\n";
        }
    }
}

namespace Part3 {
    enum class TxType { Deposit, Withdrawal, Transfer };

    std::string txTypeToString(TxType t) {
        switch (t) {
        case TxType::Deposit:    return "DEPOSIT   ";
        case TxType::Withdrawal: return "WITHDRAWAL";
        case TxType::Transfer:   return "TRANSFER  ";
        }
        return "UNKNOWN";
    }

    struct Transaction {
        int id;
        TxType type;
        double amount;
        int complexity;
    };

    std::queue<Transaction> txQueue;
    std::mutex queueMutex;
    std::mutex balanceMutex;
    std::mutex outputMutex;
    std::condition_variable condition;
    bool finished = false;

    double accountBalance = 10000.0;
    int approvedCount = 0;
    int rejectedCount = 0;
    const int TRANSACTION_COUNT = 12;

    void producer() {
        const double amounts[] = { 2500.0, 4000.0, 12000.0, 1800.0, 6500.0, -300.0,
                                   3200.0, 9500.0, 1500.0,  4200.0, 8000.0, 2100.0 };
        const TxType types[] = { TxType::Deposit, TxType::Withdrawal, TxType::Withdrawal,
                                   TxType::Deposit, TxType::Transfer,   TxType::Deposit,
                                   TxType::Withdrawal, TxType::Transfer, TxType::Deposit,
                                   TxType::Withdrawal, TxType::Withdrawal, TxType::Deposit };

        for (int i = 1; i <= TRANSACTION_COUNT; ++i) {
            Transaction tx{
                i,
                types[i - 1],
                amounts[i - 1],
                (i % 3) + 1
            };

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                txQueue.push(tx);
            }
            {
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cout << "[Producer] Created TX #" << std::setw(2) << tx.id
                    << " | Type: " << txTypeToString(tx.type)
                    << " | Amount: " << std::setw(8) << std::fixed << std::setprecision(2) << tx.amount << " UAH\n";
            }

            condition.notify_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            finished = true;
        }
        condition.notify_all();

        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "[Producer] All " << TRANSACTION_COUNT << " transactions generated.\n";
        }
    }

    void consumer(int consumerId) {
        while (true) {
            Transaction tx{};
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, []() {
                    return !txQueue.empty() || finished;
                    });

                if (txQueue.empty() && finished) {
                    break;
                }

                tx = txQueue.front();
                txQueue.pop();
            }

            // Перевірка безпеки транзакції поза критичною секцією черги
            std::this_thread::sleep_for(std::chrono::milliseconds(tx.complexity * 55));

            std::string statusMsg;
            double newBalance = 0.0;

            {
                std::lock_guard<std::mutex> lock(balanceMutex);
                if (tx.amount <= 0.0) {
                    statusMsg = "REJECTED (Invalid amount <= 0)";
                    ++rejectedCount;
                }
                else if (tx.type == TxType::Deposit) {
                    accountBalance += tx.amount;
                    statusMsg = "APPROVED";
                    ++approvedCount;
                }
                else {
                    if (accountBalance >= tx.amount) {
                        accountBalance -= tx.amount;
                        statusMsg = "APPROVED";
                        ++approvedCount;
                    }
                    else {
                        statusMsg = "REJECTED (Insufficient funds)";
                        ++rejectedCount;
                    }
                }
                newBalance = accountBalance;
            }

            {
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cout << "[Consumer " << consumerId << "] Processed TX #" << std::setw(2) << tx.id
                    << " (" << txTypeToString(tx.type) << ", "
                    << std::setw(8) << std::fixed << std::setprecision(2) << tx.amount << " UAH)"
                    << " -> " << statusMsg
                    << " | Balance: " << std::setw(9) << newBalance << " UAH\n";
            }
        }

        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "[Consumer " << consumerId << "] Finished work.\n";
        }
    }

    void run() {
        std::cout << "========================================================\n";
        std::cout << "PART 3: PRODUCER-CONSUMER (Variant 8: Bank Transactions)\n";
        std::cout << "========================================================\n";
        std::cout << "Initial Account Balance: " << std::fixed << std::setprecision(2) << accountBalance << " UAH\n\n";

        std::thread prod(producer);
        std::thread cons1(consumer, 1);
        std::thread cons2(consumer, 2);

        prod.join();
        cons1.join();
        cons2.join();

        std::cout << "\n--- Summary ---\n";
        std::cout << "Total transactions : " << TRANSACTION_COUNT << "\n";
        std::cout << "Approved           : " << approvedCount << "\n";
        std::cout << "Rejected           : " << rejectedCount << "\n";
        std::cout << "Final Balance      : " << accountBalance << " UAH\n\n";
    }
}

namespace Part4 {
    std::vector<size_t> findIndicesSequential(const std::vector<int>& data, int target) {
        std::vector<size_t> indices;
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] == target) {
                indices.push_back(i);
            }
        }
        return indices;
    }

    std::vector<size_t> findIndicesBlock(const std::vector<int>& data, int target, size_t startIdx, size_t endIdx) {
        std::vector<size_t> localIndices;
        for (size_t i = startIdx; i < endIdx; ++i) {
            if (data[i] == target) {
                localIndices.push_back(i);
            }
        }
        return localIndices;
    }

    std::vector<size_t> findIndicesParallel(const std::vector<int>& data, int target, size_t numTasks) {
        std::vector<std::future<std::vector<size_t>>> futures;
        futures.reserve(numTasks);

        size_t n = data.size();
        size_t chunkSize = n / numTasks;
        size_t remainder = n % numTasks;

        size_t currentStart = 0;
        for (size_t t = 0; t < numTasks; ++t) {
            size_t currentEnd = currentStart + chunkSize + (t < remainder ? 1 : 0);
            futures.push_back(std::async(
                std::launch::async,
                findIndicesBlock,
                std::cref(data),
                target,
                currentStart,
                currentEnd
            ));
            currentStart = currentEnd;
        }

        std::vector<size_t> allIndices;
        for (auto& fut : futures) {
            std::vector<size_t> part = fut.get();
            allIndices.insert(allIndices.end(), part.begin(), part.end());
        }
        return allIndices;
    }

    void run() {
        std::cout << "========================================================\n";
        std::cout << "PART 4: std::async & std::future (Variant 8: Find All Indices)\n";
        std::cout << "========================================================\n";

        const std::vector<size_t> sizes = { 10'000'000, 50'000'000, 100'000'000 };
        const std::vector<size_t> taskCounts = { 2, 4, 8 };
        const int targetValue = 42;

        for (size_t n : sizes) {
            std::cout << "\nDataset size N = " << n << " elements (target = " << targetValue << ")\n";
            std::vector<int> data(n);
            for (size_t i = 0; i < n; ++i) {
                data[i] = static_cast<int>(((i * 1664525ULL + 1013904223ULL) >> 16) % 1000);
            }

            auto t_s_start = std::chrono::steady_clock::now();
            std::vector<size_t> seqResult = findIndicesSequential(data, targetValue);
            auto t_s_end = std::chrono::steady_clock::now();
            double ts_ms = std::chrono::duration<double, std::milli>(t_s_end - t_s_start).count();

            std::cout << "  Sequential: Found = " << seqResult.size()
                << " | Time Ts = " << std::fixed << std::setprecision(2) << ts_ms << " ms\n";

            for (size_t k : taskCounts) {
                auto t_p_start = std::chrono::steady_clock::now();
                std::vector<size_t> parResult = findIndicesParallel(data, targetValue, k);
                auto t_p_end = std::chrono::steady_clock::now();
                double tp_ms = std::chrono::duration<double, std::milli>(t_p_end - t_p_start).count();

                bool match = (seqResult == parResult);
                double speedup = ts_ms / tp_ms;
                double efficiency = speedup / static_cast<double>(k);

                std::cout << "  Parallel (" << k << " tasks): Found = " << parResult.size()
                    << " | Time Tp = " << std::setw(6) << tp_ms << " ms"
                    << " | Speedup S = " << std::setprecision(2) << speedup << "x"
                    << " | Eff E = " << std::setprecision(2) << efficiency
                    << " | Match: " << (match ? "YES" : "NO") << "\n";
            }
        }
    }
}

int main() {
    Part1::run();
    Part2::run();
    Part3::run();
    Part4::run();
    return 0;
}