#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <filesystem>
#include <regex>

using namespace std;
namespace fs = std::filesystem;

const int oj_exclude_num = 10;
string oj_exclude[oj_exclude_num] = {
    "Draft Code",
    "Atcoder",
    ".git",
    ".github",
    "Lesson",
    "hfoj.net",
    "Others",
    "Owning",
    "vim",
    "work"
};

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
    std::string pattern = R"(^(.*?)_ver\.(.*?)_(.*?)(?:_((?:[^\d_]+(?:_[^\d_]+)*)))?_?((\d+)pt)?((?:_(\d+))*)?(\.cpp|\.in|\.out|\.ans)$)";
    std::regex regex(pattern);
    std::smatch match;
    if (std::regex_match(filename, match, regex)) {
        fn.id = match.str(1);
        fn.date = match.str(2);
        fn.status = match.str(3);
        if (!match.str(4).empty()) {
            fn.status = fn.status + "_" + match.str(4);
        }
        fn.pt = match.str(6); // $5 是带有后缀pt的分数, $6 是没有后缀的 
        fn.count = match.str(8);
        
    } else {
        // 没有找到
        #ifndef OUT_Markdown
        cout << "Warning: " << filename << " misses the pattern" << endl;
        #endif
    }
    return fn;

}
/*
FileName parseFileName(const string& filename) {
    FileName fn;
    size_t pos = filename.find("_ver.");
    if (pos == string::npos) {
        // cout << "parse: " << filename << " exit before #1" << endl;
        return fn;
    }
    fn.id = filename.substr(0, pos);
    size_t pos2 = filename.find('_', pos + 5);
    if (pos2 == string::npos) {
        pos2 = filename.find('.', pos + 5);
    }
    if (pos2 == string::npos) {
        // cout << "parse: " << filename << " exit before #2" << endl;
        return fn;
    }
    fn.date = filename.substr(pos + 4, pos2 - pos - 4);
    pos = filename.find('_', pos2 + 1);
    if (pos == string::npos) {
        pos = filename.find('.', pos2 + 1);
    }
    if (pos == string::npos) {
        // cout << "parse: " << filename << " exit before #3" << endl;
        return fn;
    }
    fn.status = filename.substr(pos2 + 1, pos - pos2 - 1);
    pos2 = filename.find("pt", pos + 1);
    if (pos2 == string::npos) {
        // cout << "parse: " << filename << " exit before #4 (status=" << fn.status << ")" << endl;
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
    // cout << "parse: " << filename << " fn.status: " << fn.status << endl;
    return fn;
}
*/

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
                    string ojName = getOJName(entry.path().parent_path().string());
                    bool flag = false;
                    for (int i = 0; i < oj_exclude_num; i++) {
                        if (ojName == oj_exclude[i]) {
                            flag = true;
                            break;
                        }
                    }
                    if (!flag) oj_map[ojName][id].push_back(filename);
                }
            }
            else if (isCppFile(filename)) {
                FileName fn = parseFileName(filename);
                if (!fn.id.empty()) {
                    string ojName = getOJName(entry.path().parent_path().string());
                    bool flag = false;
                    for (int i = 0; i < oj_exclude_num; i++) {
                        if (ojName == oj_exclude[i]) {
                            flag = true;
                            break;
                        }
                    }
                    if (!flag) oj_map[ojName][fn.id].push_back(filename);
                }
            }
        }
    }
    for (const auto& oj : oj_map) {
        for (const auto& problem : oj.second) {
            string id = problem.first;
            vector<string> cpp_files;
            string status = "";
            int max_pt = -1;
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
                    // cout << "filename: " << filename << " " << fn.status << endl;
                    if (fn.id == id) {
                        // cpp_files.push_back(oj.first + "/" + fs::absolute(filename).string());
                        cpp_files.push_back(oj.first + "/" + filename);
                        if (fn.status.empty()) {
                            // cout << "Warning: " << filename << " status empty()" << endl;
                        }
                        if (fn.status == "AC") {
                            // cout << "!En1" << endl;
                            status = "AC";
                        }
                        else if (status != "AC" && !fn.status.empty()) {
                            status = fn.status;
                        }
                        /*
                        else if (status.empty() || fn.date > parseFileName(status).date ||
                            (fn.date == parseFileName(status).date && (fn.count.empty() || safe_stoi(fn.count) > safe_stoi(parseFileName(status).count)))) {
                            status = filename;
                        }
                        */
                        // if (fn.pt == "AC") { // ??? 
                        if (fn.status == "AC") {
                            max_pt = 100;
                        }
                        else {
                            max_pt = max(max_pt, safe_stoi(fn.pt));
                        }
                        count++;
                    }
                }
            }
            // cout << "status: " << status << "|" << status.length() << endl;
            #ifdef OUT_Markdown
            cout << oj.first << " | " << id << " | " << count << " | ";
            #else
            cout << "#" << oj.first << "," << id << "," << count << ",";
            #endif
            if (status.empty()) {
                #ifdef OUT_Markdown
                cout << "N/A | ";
                #else
                cout << "N/A,";
                #endif
            }
            else {
                // FileName fn = parseFileName(status); // ??? what are you doing?
                if (status == "AC") {
                    #ifdef OUT_Markdown
                    cout << "AC | ";
                    #else
                    cout << "AC,";
                    #endif
                }
                else {
                    #ifdef OUT_Markdown
                    cout << status << " | ";
                    #else
                    cout << status << ",";
                    #endif
                }
            }
            // ??? 你要不要看一下你在干什么
            // if (max_pt == 0) {
            if (max_pt < 0) {
                // max_pt 默认值为 -1
                #ifdef OUT_Markdown
                cout << "N/A | ";
                #else
                cout << "N/A,";
                #endif
            }
            else {
                #ifdef OUT_Markdown
                cout << max_pt << " | ";
                #else
                cout << max_pt << ",";
                #endif
            }
            #ifdef OUT_Markdown
            cout << data_points.size() << " | ";
            #else
            cout << data_points.size() << endl;
            cout << "  cpp files: ";
            #endif
            #ifdef OUT_Markdown
            bool first_out = true; // 保证最后没有多余的逗号
            #endif
            for (const auto& cpp_file : cpp_files) {
                #ifdef OUT_Markdown
                if (first_out) cout << cpp_file;
                else cout << ", " << cpp_file;
                first_out = false;
                #else
                cout << cpp_file << " ";
                #endif
            }
            #ifdef OUT_Markdown
            cout << " | ";
            #else
            cout << endl;
            cout << "  data points: ";
            #endif
            #ifdef OUT_Markdown
            first_out = true;
            #endif
            for (const auto& data_point : data_points) {
                #ifdef OUT_Markdown
                if (first_out) cout << data_point;
                else cout << ", " << data_point;
                first_out = false;
                #else
                cout << data_point << " ";
                #endif
            }
            #ifdef OUT_Markdown
            #ifdef OUT_Checkbox
            cout << " | " << (status == "AC" ? "<ul><li>[x]</li></ul>" : "<ul><li>[ ]</li></ul>");
            #endif
            #endif
            cout << endl;
        }
    }
}

int main() {
#ifdef OUT_Markdown
    // 输出表头
    cout << "OJ | ID | 提交次数 | 最终提交状态 | 最高分数 | 数据点数量 | 代码提交文件 | 数据点文件";
    #ifdef OUT_Checkbox
    cout << " | 完成";
    #endif
    cout << endl;
    cout << "-- | -- | ------- | ------------ | ------ | --------- | ------------ | ---------";
    #ifdef OUT_Checkbox
    cout << " | ---";
    #endif
    cout << endl;
#endif
#ifndef CI
    processDirectory("D:\\code-backup\\");
#else
    processDirectory("code-backup");
#endif
    return 0;
}
