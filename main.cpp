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

const string ColorAcceptedRGB = "#52C41A";
const string ColorUnacceptedRGB = "#E74C3C";
const string ColorDefaultRGB = "#FFFFFF";

const int oj_exclude_num = 10;
const string repo_url = "https://github.com/bitsstdcheee/code-backup";
const string repo_branch = "development";
const string repo_prefix = repo_url + "/blob/" + repo_branch + "/";
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
// 输出时输出关键内容
string getFileDescription(const FileName& fn, bool enable_AC = false) {
    string res = fn.date + " " + fn.status;
    if (!fn.pt.empty()) {
        res = res + "/" + fn.pt;
    }
    if (enable_AC) {
        if (fn.status == "AC") {
            res = res + " :white_check_mark:";
        }
    }
    if (safe_stoi(fn.count) > 0) {
        // 存在重复计数编号则在后面注明
        res = res + " (" + fn.count + ")";
    }
    return res;
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

// 将文本直接插入 Katex 代码前进行必要的转义
// 在某些环境的嵌套下可能需要双层转义, 此时打开 double_mode
string KatexFormat(const string& str, bool double_mode = false) {
    string res = str;
    {
        // 反斜杠
        std::regex pattern(R"(\\)");
        string replace_string(double_mode ? R"(\\backslash)" : R"(\backslash)");
        res = std::regex_replace(res, pattern, replace_string);
    }
    {
        // 百分号
        std::regex pattern(R"(%)");
        string replace_string(double_mode ? R"(\\%)" : R"(\%)");
        res = std::regex_replace(res, pattern, replace_string);
    }
    {
        // 下划线
        std::regex pattern(R"(_)");
        string replace_string(double_mode ? R"(\\_)" : R"(\_)");
        res = std::regex_replace(res, pattern, replace_string);
    }
    return res;
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
            vector<pair<FileName, string>> cpp_files;
            string status = "";
            string current_oj = oj.first;
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
                    if (fn.id == id) {
                        cpp_files.push_back(make_pair(fn, oj.first + "/" + filename));
                        if (fn.status.empty()) {

                        }
                        if (fn.status == "AC") {
                            // cout << "!En1" << endl;
                            status = "AC";
                        }
                        else if (status != "AC" && !fn.status.empty()) {
                            status = fn.status;
                        }
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
            #ifdef OUT_Markdown
            cout << current_oj << " | " ;
            #ifdef OUT_ColorAC
            cout << "$\\textcolor{" ;
            if (status == "AC") cout << ColorAcceptedRGB;
            else if (status == "Waiting") cout << ColorDefaultRGB;  // Waiting 状态时 color 留空以保持默认颜色
            else cout << ColorUnacceptedRGB;
            cout << "}{\\text{" << KatexFormat(id, 
            #ifdef OUT_DoubleBackslash
            true
            #else
            false
            #endif
                ) << "}}$ | " << count << " | ";
            #else
            cout << id << " | " << count << " | ";
            #endif
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
                if (first_out) cout << "[" << getFileDescription(cpp_file.first, true) << "](" << repo_prefix << cpp_file.second << ")";
                else cout << "<br>[" << getFileDescription(cpp_file.first, true) << "](" << repo_prefix << cpp_file.second << ")";
                first_out = false;
                #else
                cout << cpp_file.second << " ";
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
                if (first_out) cout << "[" << data_point << "](" << repo_prefix << current_oj << "/" << id << "_" << data_point << ")";
                else cout << "<br>[" << data_point << "](" << repo_prefix << current_oj << "/" << id << "_" << data_point << ")";
                first_out = false;
                #else
                cout << data_point << " ";
                #endif
            }
            #ifdef OUT_Markdown
            #ifdef OUT_Checkbox
            cout << " | " << (status == "AC" ? "<ul><li>[x] 完成</li></ul>" : "<ul><li>[ ] 未完成</li></ul>");
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
    cout << endl;
    cout << "-- | -- | ------- | ------------ | ------ | --------- | ------------ | ---------";
    cout << endl;
#endif
#ifndef CI
    processDirectory("D:\\code-backup\\");
#else
    processDirectory("code-backup");
#endif
    return 0;
}
