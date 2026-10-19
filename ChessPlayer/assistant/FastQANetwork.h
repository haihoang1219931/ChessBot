#ifndef FASTQANETWORK_H
#define FASTQANETWORK_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

struct QAPair {
    std::string question;
    std::string answer;
};

class FastQANetwork
{
public:
    FastQANetwork();
    bool load_from_file(const std::string& filepath);
    std::string query(const std::string& user_input);
private:
    int get_levenshtein_distance(const std::string& s1, const std::string& s2);
    std::string clean_string(const std::string& input);
    std::unordered_map<std::string, std::string> exact_map;
    std::vector<QAPair> raw_list;
};

#endif // FASTQANETWORK_H
