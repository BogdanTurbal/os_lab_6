#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <chrono>

const int n = 120, m = 100, k = 120;

// initialize matrices A, B, and C
std::vector<std::vector<int>> A(n, std::vector<int>(m));
std::vector<std::vector<int>> B(m, std::vector<int>(k));
std::vector<std::vector<int>> C(n, std::vector<int>(k));

void compute_element(int i, int j) {
    int result = 0;
    for (int x = 0; x < m; ++x) {
        result += A[i][x] * B[x][j];
    }
    C[i][j] = result;
}

void compute_segment(int start_row, int end_row) {
    for (int i = start_row; i < end_row; ++i) {
        for (int j = 0; j < k; ++j) {
            compute_element(i, j);
        }
    }
}

double matrix_multiply_threads(int num_threads) {
    std::vector<std::thread> threads;
    int rows_per_thread = n / num_threads;
    int remainder_rows = n % num_threads;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; ++i) {
        int start_row = i * rows_per_thread;
        int end_row = (i == num_threads - 1) ? start_row + rows_per_thread + remainder_rows : start_row + rows_per_thread;
        threads.emplace_back(compute_segment, start_row, end_row);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    return elapsed.count();
}

int exp_1() {
    // seed random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 9);

    // initialize matrices A and B with random values
    for (auto& row : A) for (int& cell : row) cell = distrib(gen);
    for (auto& row : B) for (int& cell : row) cell = distrib(gen);

    std::vector<int> thread_counts = {1, 10, 12, 100, 500, 1000, 10000, n*k};
    for (int count : thread_counts) {
        double time_taken = matrix_multiply_threads(count);
        std::cout << "Time taken with " << count << " threads: " << time_taken << " seconds." << std::endl;
    }

    return 0;
}

int shared_variable = 0;
std::mutex mtx;
std::mutex mtx_l;

void increment_without_lock(int num_iterations) {
    for (int i = 0; i < num_iterations; ++i) {
        ++shared_variable;
    }
}

void increment_with_lock(int num_iterations) {
    for (int i = 0; i < num_iterations; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        ++shared_variable;
    }
}

int exp_2() {
    const int num_iterations = 1000000;

    // scenario 1: Without Lock
    shared_variable = 0;
    std::thread thread1(increment_without_lock, num_iterations);
    std::thread thread2(increment_without_lock, num_iterations);
    auto start_time = std::chrono::high_resolution_clock::now();
    thread1.join();
    thread2.join();
    auto end_time = std::chrono::high_resolution_clock::now();
    int result_without_lock = shared_variable;
    double time_without_lock = std::chrono::duration<double>(end_time - start_time).count();

    // reset shared variable
    shared_variable = 0;

    // scenario 2: With Lock
    std::thread thread3(increment_with_lock, num_iterations);
    std::thread thread4(increment_with_lock, num_iterations);
    start_time = std::chrono::high_resolution_clock::now();
    thread3.join();
    thread4.join();
    end_time = std::chrono::high_resolution_clock::now();
    int result_with_lock = shared_variable;
    double time_with_lock = std::chrono::duration<double>(end_time - start_time).count();

    // output results
    std::cout << "Result without lock: " << result_without_lock << ", Time taken: " << time_without_lock << " seconds\n";
    std::cout << "Result with lock: " << result_with_lock << ", Time taken: " << time_with_lock << " seconds\n";

    return 0;
}

void increment_with_batch(int num_iterations, int batch_size) {
    int local_sum = 0;
    for (int i = 0; i < num_iterations; ++i) {
        local_sum += 1;
        if (local_sum == batch_size) {
            std::lock_guard<std::mutex> lock(mtx);
            shared_variable += local_sum;
            local_sum = 0;
        }
    }
    // add any remaining counts that didn't make up a full batch
    if (local_sum > 0) {
        std::lock_guard<std::mutex> lock(mtx);
        shared_variable += local_sum;
    }
}

int optimized_exp_2() {
    const int num_iterations = 1000000;
    const int batch_size = 10000;

    // reset shared variable
    shared_variable = 0;

    std::thread thread1(increment_with_batch, num_iterations, batch_size);
    std::thread thread2(increment_with_batch, num_iterations, batch_size);
    auto start_time = std::chrono::high_resolution_clock::now();
    thread1.join();
    thread2.join();
    auto end_time = std::chrono::high_resolution_clock::now();
    int result_with_batch = shared_variable;
    double time_with_batch = std::chrono::duration<double>(end_time - start_time).count();

    // output results
    std::cout << "Result with batched lock: " << result_with_batch << ", Time taken: " << time_with_batch << " seconds\n";

    return 0;
}



void increment_without_lock_context_switch(int num_iterations) {
    for (int i = 0; i < num_iterations; ++i) {
        int value = shared_variable;
        for (int j = 0; j < 1000000; ++j) {}
        shared_variable = value + 1;
    }
}

int fully_synchronized_exp_2() {
    const int num_iterations = 1000;

    // scenario 1: Without Lock
    shared_variable = 0;
    std::thread thread1(increment_without_lock_context_switch, num_iterations);
    std::thread thread2(increment_without_lock_context_switch, num_iterations);
    auto start_time = std::chrono::high_resolution_clock::now();
    thread1.join();
    thread2.join();
    auto end_time = std::chrono::high_resolution_clock::now();
    int result_without_lock = shared_variable;
    double time_without_lock = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "Result: " << result_without_lock << ", Time taken: " << time_without_lock << " seconds\n";

    return 0;
}
int main(){
    //int res_1 = exp_1();
    //int res_2 = exp_2();
    //shared_variable = 0;
    //int res_3 = optimized_exp_2();
    int res_3_1 = fully_synchronized_exp_2();
    return 0;
}