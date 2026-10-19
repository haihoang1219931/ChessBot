#include "FastQANetwork.h"

FastQANetwork::FastQANetwork()
{

}

int FastQANetwork::get_levenshtein_distance(const std::string& s1, const std::string& s2) {
    int m = s1.size();
    int n = s2.size();

    // Allocate the 2D matrix
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    // Correctly initialize base cases for the matrix boundaries
    for (int i = 0; i <= m; i++) dp[i][0] = i;
    for (int j = 0; j <= n; j++) dp[0][j] = j;

    // Fill the rest of the matrix
    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }
    return dp[m][n];
}

std::string FastQANetwork::clean_string(const std::string& input) {
    std::string result = "";
    for (char c : input) {
        // Keep letters and spaces, convert to lowercase
        if (std::isalnum(c) || std::isspace(c)) {
            result += std::tolower(c);
        }
    }
    // Trim trailing/leading spaces if necessary (optional)
    return result;
}

bool FastQANetwork::load_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "CRITICAL ERROR: Could not open chess database: " << filepath << "\n";
        return false;
    }
    std::cout << "load_from_file no error" << std::endl;
    // Optimization: Pre-allocate space for 1000 items to avoid vector resizing delays
    exact_map.reserve(1050);
    raw_list.reserve(1050);
    std::cout << "Pre-allocate space for 1000 items" << std::endl;
    std::string line;
    int loaded_count = 0;

    while (std::getline(file, line)) {
        // Skip empty lines or carriage returns
        if (line.empty() || line == "\r") continue;

        // Locate the strict Tab separator
        size_t tab_pos = line.find('\t');
        if (tab_pos != std::string::npos) {
            std::string raw_question = line.substr(0, tab_pos);
            std::string answer = line.substr(tab_pos + 1);

            // Strip out trailing '\r' if the file was created on Windows
            if (!answer.empty() && answer.back() == '\r') {
                answer.pop_back();
            }

            // Standardize the question text for consistent O(1) tracking
            std::string cleaned_question = clean_string(raw_question);

            // Save to our high-performance lookups
            exact_map[cleaned_question] = answer;
            raw_list.push_back({cleaned_question, answer});

            loaded_count++;
        }
    }
    file.close();
    std::cout << "[✅ System Ready] Loaded " << loaded_count << " chess intents into cache memory.\n";
    return true;
}

// Call this inside your main loop to process incoming user queries
std::string FastQANetwork::query(const std::string& user_input) {
    // Clean user input exactly the same way we cleaned the file questions
    std::string cleaned_input = clean_string(user_input);

    // Try exact match lookup
    auto it = exact_map.find(cleaned_input);
    if (it != exact_map.end()) {
        return it->second;
    }

    // Fallback to your fuzzy Levenshtein loop here if exact match fails...
    return "Are you trying to flag me? Type a valid chess question.";
}
