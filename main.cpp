#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// 定义文件名中的关键词
struct FileName {
    string id;
    string date;
    string status;
    string pt;
    string count;
    string data_point;
};
// 安全地将字符串转换为整数，如果转换失败则返回0
int safe_stoi(const string& str) {
    try {
        return stoi(str);
    }
    catch (const std::invalid_argument&) {
        return 0;
    }
}
// 解析文件名，提取关键词
FileName parseFileName(const string& filename) {
    FileName fn;
    size_t pos = filename.find("_ver.");
    if (pos == string::npos) {
        return fn;
    }
    fn.id = filename.substr(0, pos);
    size_t pos2 = filename.find('_', pos + 5);
    if (pos2 == string::npos) {
        pos2 = filename.find('.', pos + 5);
    }
    if (pos2 == string::npos) {
        return fn;
    }
    fn.date = filename.substr(pos + 4, pos2 - pos - 4);
    pos = filename.find('_', pos2 + 1);
    if (pos == string::npos) {
        pos = filename.find('.', pos2 + 1);
    }
    if (pos == string::npos) {
        return fn;
    }
    fn.status = filename.substr(pos2 + 1, pos - pos2 - 1);
    pos2 = filename.find("pt", pos + 1);
    if (pos2 == string::npos) {
        return fn;
    }
    fn.pt = filename.substr(pos + 1, pos2 - pos - 1);
    pos = filename.find('_', pos2 + 2);
    if (pos != string::npos) {
        pos2 = filename.find('.', pos + 1);
        if (pos2 != string::npos) {
            fn.count = filename.substr(pos + 1, pos2 - pos - 1);
        }
    }
    pos = filename.find('_', pos2 + 1);
    if (pos != string::npos) {
        pos2 = filename.find('.', pos + 1);
        if (pos2 != string::npos) {
            fn.data_point = filename.substr(pos + 1, pos2 - pos - 1);
        }
    }
    return fn;
}

// 判断文件是否是数据文件
bool isDataFile(const string& filename) {
    return filename.size() > 4 && (filename.substr(filename.size() - 3) == ".in" ||
        filename.substr(filename.size() - 4) == ".out" || filename.substr(filename.size() - 4) == ".ans");
}

// 判断文件是否是源代码文件
bool isCppFile(const string& filename) {
    return filename.size() > 4 && filename.substr(filename.size() - 4) == ".cpp";
}

// 获取文件的OJ名称
string getOJName(const string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == string::npos) {
        return "";
    }
    return path.substr(pos + 1);
}

// 统计每个OJ中每道题目是否通过，历史分数，提交次数，代码文件（以文件链接输出），这道题的数据点（如有则输出）
void processDirectory(const string& path) {
    map<string, map<string, vector<string>>> oj_map;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            string filename = entry.path().filename().string();
            if (isDataFile(filename)) {
                size_t pos = filename.find_last_of('_');
                if (pos != string::npos) {
                    string id = filename.substr(0, pos);
                    oj_map[getOJName(entry.path().parent_path().string())][id].push_back(filename);
                }
            }
            else if (isCppFile(filename)) {
                FileName fn = parseFileName(filename);
                if (!fn.id.empty()) {
                    oj_map[getOJName(entry.path().parent_path().string())][fn.id].push_back(filename);
                }
            }
        }
    }
    for (const auto& oj : oj_map) {
        for (const auto& problem : oj.second) {
            string id = problem.first;
            vector<string> cpp_files;
            string status = "";
            int max_pt = 0;
            int count = 0;
            vector<string> data_points;
            for (const auto& filename : problem.second) {
                if (isDataFile(filename)) {
                    size_t pos = filename.find_last_of('_');
                    if (pos != string::npos) {
                        data_points.push_back(filename.substr(pos + 1));
                    }
                }
                else if (isCppFile(filename)) {
                    FileName fn = parseFileName(filename);
                    if (fn.id == id) {
                        cpp_files.push_back(fs::absolute(filename).string());
                        if (fn.status == "AC") {
                            status = "AC";
                        }
                        else if (status.empty() || fn.date > parseFileName(status).date ||
                            (fn.date == parseFileName(status).date && (fn.count.empty() || safe_stoi(fn.count) > safe_stoi(parseFileName(status).count)))) {
                            status = filename;
                        }
                        if (fn.pt == "AC") {
                            max_pt = 100;
                        }
                        else {
                            max_pt = max(max_pt, safe_stoi(fn.pt));
                        }
                        count++;
                    }
                }
            }
            cout << oj.first << "," << id << "," << count << ",";
            if (status.empty()) {
                cout << "N/A,";
            }
            else {
                FileName fn = parseFileName(status);
                if (fn.status == "AC") {
                    cout << "AC,";
                }
                else {
                    cout << fn.status << ",";
                }
            }
            if (max_pt == 0) {
                cout << "N/A,";
            }
            else {
                cout << max_pt << ",";
            }
            cout << data_points.size() << endl;
            cout << "  cpp files: ";
            for (const auto& cpp_file : cpp_files) {
                cout << cpp_file << " ";
            }
            cout << endl;
            cout << "  data points: ";
            for (const auto& data_point : data_points) {
                cout << data_point << " ";
            }
            cout << endl;
        }
    }
}

int main() {
#ifndef CI
    processDirectory("D:\\code-backup\\");
#else
    processDirectory("code-backup");
#endif
    return 0;
}
