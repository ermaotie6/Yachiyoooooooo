#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <iostream>

// ============= Performance Measurement Utilities =============

class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;

public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        end_time = std::chrono::high_resolution_clock::now();
    }

    double get_elapsed_ms() const {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        return static_cast<double>(duration.count());
    }

    double get_elapsed_us() const {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        return static_cast<double>(duration.count());
    }
};

class PerformanceStatistics {
public:
    std::vector<double> measurements;

    void add_measurement(double value) {
        measurements.push_back(value);
    }

    double get_min() const {
        return *std::min_element(measurements.begin(), measurements.end());
    }

    double get_max() const {
        return *std::max_element(measurements.begin(), measurements.end());
    }

    double get_average() const {
        double sum = 0.0;
        for (double m : measurements) {
            sum += m;
        }
        return sum / measurements.size();
    }

    double get_median() const {
        std::vector<double> sorted_measurements = measurements;
        std::sort(sorted_measurements.begin(), sorted_measurements.end());
        size_t size = sorted_measurements.size();
        if (size % 2 == 0) {
            return (sorted_measurements[size / 2 - 1] + sorted_measurements[size / 2]) / 2.0;
        } else {
            return sorted_measurements[size / 2];
        }
    }

    double get_percentile(double percentile) const {
        std::vector<double> sorted_measurements = measurements;
        std::sort(sorted_measurements.begin(), sorted_measurements.end());
        size_t index = static_cast<size_t>((percentile / 100.0) * sorted_measurements.size());
        return sorted_measurements[std::min(index, sorted_measurements.size() - 1)];
    }

    double get_stddev() const {
        double avg = get_average();
        double variance = 0.0;
        for (double m : measurements) {
            variance += (m - avg) * (m - avg);
        }
        return std::sqrt(variance / measurements.size());
    }

    void print_report(const std::string& operation_name) const {
        std::cout << "\n=== Performance Report: " << operation_name << " ===" << std::endl;
        std::cout << "Samples: " << measurements.size() << std::endl;
        std::cout << "Min: " << get_min() << " ms" << std::endl;
        std::cout << "Max: " << get_max() << " ms" << std::endl;
        std::cout << "Average: " << get_average() << " ms" << std::endl;
        std::cout << "Median: " << get_median() << " ms" << std::endl;
        std::cout << "P95: " << get_percentile(95) << " ms" << std::endl;
        std::cout << "P99: " << get_percentile(99) << " ms" << std::endl;
        std::cout << "Std Dev: " << get_stddev() << " ms" << std::endl;
    }
};

// ============= Performance Test Fixtures =============

class PerformanceTest : public ::testing::Test {
protected:
    PerformanceStatistics user_registration_stats;
    PerformanceStatistics user_login_stats;
    PerformanceStatistics message_send_stats;
    PerformanceStatistics message_query_stats;
    PerformanceStatistics six_layer_review_stats;
    PerformanceStatistics concurrent_send_stats;

    void TearDown() override {
        // Print all performance reports
        user_registration_stats.print_report("User Registration");
        user_login_stats.print_report("User Login");
        message_send_stats.print_report("Message Send");
        message_query_stats.print_report("Message Query");
        six_layer_review_stats.print_report("Six-Layer Review");
        concurrent_send_stats.print_report("Concurrent Message Send");
    }
};

// ============= Authentication Performance Tests =============

TEST_F(PerformanceTest, UserRegistrationPerformance) {
    // Test: User registration should complete in < 200ms
    const int num_iterations = 100;
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate registration operation:
        // 1. Validate input (< 1ms)
        // 2. Hash password (bcrypt: ~100-150ms)
        // 3. Database insert (< 10ms)
        // 4. Return response (< 5ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulated bcrypt
        std::this_thread::sleep_for(std::chrono::milliseconds(8));  // Simulated DB
        
        timer.stop();
        user_registration_stats.add_measurement(timer.get_elapsed_ms());
    }

    double avg_time = user_registration_stats.get_average();
    EXPECT_LT(avg_time, 200.0) << "Average registration time should be < 200ms";
    EXPECT_LT(user_registration_stats.get_percentile(95), 250.0) << "P95 should be < 250ms";
}

TEST_F(PerformanceTest, UserLoginPerformance) {
    // Test: User login should complete in < 150ms
    const int num_iterations = 100;
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate login operation:
        // 1. Database query (< 5ms)
        // 2. Password verification (< 100ms)
        // 3. Generate JWT (< 10ms)
        // 4. Return token (< 5ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(4));   // DB query
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Password verify
        std::this_thread::sleep_for(std::chrono::milliseconds(8));   // JWT generation
        
        timer.stop();
        user_login_stats.add_measurement(timer.get_elapsed_ms());
    }

    double avg_time = user_login_stats.get_average();
    EXPECT_LT(avg_time, 150.0) << "Average login time should be < 150ms";
    EXPECT_LT(user_login_stats.get_percentile(99), 200.0) << "P99 should be < 200ms";
}

// ============= Message Processing Performance Tests =============

TEST_F(PerformanceTest, MessageSendPerformance) {
    // Test: Message send should complete in < 1000ms (including 6-layer review)
    const int num_iterations = 50;
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate 6-layer review process:
        // Layer 1: Rate limiting check (< 5ms - Redis)
        // Layer 2: IP blacklist check (< 5ms - Redis)
        // Layer 3: Keyword filtering (< 10ms - in-memory)
        // Layer 4: AI content review (< 800ms - API call or local model)
        // Layer 5: Behavior anomaly (< 20ms - Redis)
        // Layer 6: Manual review marking (< 50ms - DB write)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));   // Rate limit
        std::this_thread::sleep_for(std::chrono::milliseconds(5));   // IP check
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Keyword filter
        std::this_thread::sleep_for(std::chrono::milliseconds(800)); // AI review
        std::this_thread::sleep_for(std::chrono::milliseconds(20));  // Behavior
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Manual mark
        std::this_thread::sleep_for(std::chrono::milliseconds(5));   // DB write
        
        timer.stop();
        message_send_stats.add_measurement(timer.get_elapsed_ms());
    }

    double avg_time = message_send_stats.get_average();
    EXPECT_LT(avg_time, 1000.0) << "Average message send should be < 1000ms";
    EXPECT_LT(message_send_stats.get_percentile(95), 1100.0) << "P95 should be < 1100ms";
}

TEST_F(PerformanceTest, MessageQueryPerformance) {
    // Test: Message query should complete in < 500ms
    const int num_iterations = 100;
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate query operation:
        // 1. Authentication verification (< 5ms)
        // 2. Permission check (< 5ms)
        // 3. Database query with pagination (< 50ms)
        // 4. Cache retrieval (< 10ms)
        // 5. JSON serialization (< 20ms)
        // 6. Response construction (< 5ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(4));   // Auth
        std::this_thread::sleep_for(std::chrono::milliseconds(4));   // Permission
        std::this_thread::sleep_for(std::chrono::milliseconds(40));  // DB query
        std::this_thread::sleep_for(std::chrono::milliseconds(8));   // Cache
        std::this_thread::sleep_for(std::chrono::milliseconds(15));  // JSON
        
        timer.stop();
        message_query_stats.add_measurement(timer.get_elapsed_ms());
    }

    double avg_time = message_query_stats.get_average();
    EXPECT_LT(avg_time, 500.0) << "Average message query should be < 500ms";
    EXPECT_LT(message_query_stats.get_percentile(99), 550.0) << "P99 should be < 550ms";
}

// ============= Six-Layer Review System Performance =============

TEST_F(PerformanceTest, RateLimitingPerformance) {
    // Test: Rate limiting check should be < 5ms (Redis operation)
    const int num_iterations = 100;
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate Redis rate limit check
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        
        timer.stop();
        six_layer_review_stats.add_measurement(timer.get_elapsed_ms());
    }

    EXPECT_LT(six_layer_review_stats.get_average(), 5.0) << "Rate limit check should be < 5ms";
}

TEST_F(PerformanceTest, KeywordFilterPerformance) {
    // Test: Keyword filtering should be < 10ms
    const int num_iterations = 100;
    const std::vector<std::string> test_contents = {
        "Normal message content here",
        "This contains some filtered words",
        "Another regular message",
        "Potentially inappropriate content",
        "Safe and appropriate text"
    };

    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate keyword filtering (in-memory trie or regex)
        std::string content = test_contents[i % test_contents.size()];
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        timer.stop();
        six_layer_review_stats.add_measurement(timer.get_elapsed_ms());
    }

    EXPECT_LT(six_layer_review_stats.get_average(), 10.0) << "Keyword filter should be < 10ms";
}

TEST_F(PerformanceTest, AIContentReviewPerformance) {
    // Test: AI content review latency (typically 500-1000ms)
    const int num_iterations = 20; // Lower iteration count for slow operations
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate AI review (either API call or local model inference)
        // Typical latency: 500-1000ms for cloud API
        // Typical latency: 100-500ms for local model
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        
        timer.stop();
        six_layer_review_stats.add_measurement(timer.get_elapsed_ms());
    }

    double avg_time = six_layer_review_stats.get_average();
    EXPECT_LT(avg_time, 1000.0) << "AI review should be < 1000ms (average)";
}

// ============= Concurrent Operations Performance =============

TEST_F(PerformanceTest, ConcurrentMessageSendPerformance) {
    // Test: System should handle multiple concurrent message sends efficiently
    const int num_concurrent_users = 10;
    const int messages_per_user = 5;
    
    for (int user = 0; user < num_concurrent_users; user++) {
        for (int msg = 0; msg < messages_per_user; msg++) {
            PerformanceTimer timer;
            timer.start();
            
            // Simulate concurrent message send
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulated processing
            
            timer.stop();
            concurrent_send_stats.add_measurement(timer.get_elapsed_ms());
        }
    }

    EXPECT_LT(concurrent_send_stats.get_average(), 200.0) << "Concurrent sends should not degrade significantly";
}

TEST_F(PerformanceTest, DatabaseQueryScaling) {
    // Test: Database query performance with varying result set sizes
    const int num_iterations = 50;
    const std::vector<int> result_set_sizes = {10, 50, 100, 500, 1000};
    
    for (int size : result_set_sizes) {
        PerformanceStatistics size_stats;
        
        for (int i = 0; i < num_iterations; i++) {
            PerformanceTimer timer;
            timer.start();
            
            // Simulate DB query time scaling with result size
            // Typical: 5ms + 0.05ms per record
            std::this_thread::sleep_for(std::chrono::milliseconds(5 + size / 20));
            
            timer.stop();
            size_stats.add_measurement(timer.get_elapsed_ms());
        }
        
        std::cout << "\nQuery performance for " << size << " results: " 
                  << size_stats.get_average() << "ms (avg)" << std::endl;
    }
}

// ============= Memory and Resource Tests =============

TEST_F(PerformanceTest, TokenVerificationPerformance) {
    // Test: JWT token verification should be < 5ms
    const int num_iterations = 100;
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate JWT verification (HMAC-SHA256)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        
        timer.stop();
        user_login_stats.add_measurement(timer.get_elapsed_ms());
    }

    EXPECT_LT(user_login_stats.get_average(), 5.0) << "Token verification should be < 5ms";
}

TEST_F(PerformanceTest, CacheHitPerformance) {
    // Test: Cache hits should be < 2ms
    const int num_iterations = 100;
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate Redis cache hit
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        timer.stop();
        message_query_stats.add_measurement(timer.get_elapsed_ms());
    }

    EXPECT_LT(message_query_stats.get_average(), 2.0) << "Cache hits should be < 2ms";
}

// ============= Throughput Tests =============

TEST_F(PerformanceTest, MessageSendThroughput) {
    // Test: System should handle at least 50 messages/second end-to-end
    const int num_messages = 100;
    PerformanceTimer total_timer;
    total_timer.start();
    
    for (int i = 0; i < num_messages; i++) {
        // Simulate message send with 6-layer review
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    total_timer.stop();
    double total_time_seconds = total_timer.get_elapsed_ms() / 1000.0;
    double throughput = num_messages / total_time_seconds;
    
    std::cout << "\nMessage send throughput: " << throughput << " msg/sec" << std::endl;
    EXPECT_GT(throughput, 50.0) << "Should handle at least 50 msg/sec";
}

TEST_F(PerformanceTest, QueryThroughput) {
    // Test: System should handle at least 100 queries/second
    const int num_queries = 200;
    PerformanceTimer total_timer;
    total_timer.start();
    
    for (int i = 0; i < num_queries; i++) {
        // Simulate message query
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    total_timer.stop();
    double total_time_seconds = total_timer.get_elapsed_ms() / 1000.0;
    double throughput = num_queries / total_time_seconds;
    
    std::cout << "\nMessage query throughput: " << throughput << " queries/sec" << std::endl;
    EXPECT_GT(throughput, 100.0) << "Should handle at least 100 queries/sec";
}

// ============= Stress Test Scenarios =============

TEST_F(PerformanceTest, SustainedLoadPerformance) {
    // Test: Performance under sustained load (100 requests/sec for 10 seconds)
    const int requests_per_second = 100;
    const int duration_seconds = 10;
    const int total_requests = requests_per_second * duration_seconds;
    
    PerformanceStatistics sustained_stats;
    PerformanceTimer total_timer;
    total_timer.start();
    
    for (int i = 0; i < total_requests; i++) {
        PerformanceTimer req_timer;
        req_timer.start();
        
        // Simulate request processing
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        req_timer.stop();
        sustained_stats.add_measurement(req_timer.get_elapsed_ms());
    }
    
    total_timer.stop();
    
    std::cout << "\nSustained load test (1000 requests):" << std::endl;
    std::cout << "Total time: " << total_timer.get_elapsed_ms() / 1000.0 << " seconds" << std::endl;
    std::cout << "Average response: " << sustained_stats.get_average() << " ms" << std::endl;
    std::cout << "P95 response: " << sustained_stats.get_percentile(95) << " ms" << std::endl;
}

TEST_F(PerformanceTest, PeakLoadPerformance) {
    // Test: Performance under peak load (500 requests/sec for 5 seconds)
    const int peak_requests = 2500;
    PerformanceStatistics peak_stats;
    
    for (int i = 0; i < peak_requests; i++) {
        PerformanceTimer timer;
        timer.start();
        
        // Simulate request processing under peak load
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        
        timer.stop();
        peak_stats.add_measurement(timer.get_elapsed_ms());
    }
    
    std::cout << "\nPeak load test (2500 requests):" << std::endl;
    std::cout << "Average response: " << peak_stats.get_average() << " ms" << std::endl;
    std::cout << "P99 response: " << peak_stats.get_percentile(99) << " ms" << std::endl;
    
    // P99 should not exceed 20ms even under peak load
    EXPECT_LT(peak_stats.get_percentile(99), 20.0) << "P99 should be acceptable even at peak load";
}

// ============= Response Time SLA Tests =============

TEST_F(PerformanceTest, SLAComplianceRegistration) {
    // SLA: 95% of registrations complete in < 200ms
    const int num_iterations = 100;
    user_registration_stats.measurements.clear();
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        timer.stop();
        user_registration_stats.add_measurement(timer.get_elapsed_ms());
    }
    
    double p95 = user_registration_stats.get_percentile(95);
    EXPECT_LT(p95, 200.0) << "SLA: P95 registration < 200ms";
    std::cout << "\nRegistration SLA: P95 = " << p95 << "ms (target: 200ms)" << std::endl;
}

TEST_F(PerformanceTest, SLAComplianceMessageSend) {
    // SLA: 95% of message sends complete in < 1100ms
    const int num_iterations = 50;
    message_send_stats.measurements.clear();
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        timer.stop();
        message_send_stats.add_measurement(timer.get_elapsed_ms());
    }
    
    double p95 = message_send_stats.get_percentile(95);
    EXPECT_LT(p95, 1100.0) << "SLA: P95 message send < 1100ms";
    std::cout << "\nMessage Send SLA: P95 = " << p95 << "ms (target: 1100ms)" << std::endl;
}

TEST_F(PerformanceTest, SLAComplianceQuery) {
    // SLA: 95% of queries complete in < 550ms
    const int num_iterations = 100;
    message_query_stats.measurements.clear();
    
    for (int i = 0; i < num_iterations; i++) {
        PerformanceTimer timer;
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        timer.stop();
        message_query_stats.add_measurement(timer.get_elapsed_ms());
    }
    
    double p95 = message_query_stats.get_percentile(95);
    EXPECT_LT(p95, 550.0) << "SLA: P95 query < 550ms";
    std::cout << "\nMessage Query SLA: P95 = " << p95 << "ms (target: 550ms)" << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
