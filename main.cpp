#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
#include <filesystem>
#include <regex>

using namespace std;
namespace fs = std::filesystem;

#ifdef OUT_Badge
#define OUT_HTML_Markdown
#undef OUT_ColorAC
#endif

const string ColorAcceptedRGB = "#52C41A";
const string ColorUnacceptedRGB = "#E74C3C";
const string ColorDefaultRGB = "#FFFFFF";
const string ColorRustRGB = "#DEA584";

const int oj_exclude_num = 10;
const string repo_url = "https://github.com/bitsstdcheee/code-backup";
const string repo_branch = "development";
const string repo_prefix = repo_url + "/blob/" + repo_branch + "/";

const string markdown_pre = string("[![Luogu](https://github.com/bitsstdcheee/code-backup-status/actions/workflows/luogu.yml/badge.svg)](https://github.com/bitsstdcheee/code-backup-status/actions/workflows/luogu.yml)") + " " +
string("[![Mirror](https://github.com/bitsstdcheee/code-backup-status/actions/workflows/mirror.yml/badge.svg)](https://github.com/bitsstdcheee/code-backup-status/actions/workflows/mirror.yml)") + " " +
string("[![Runner](https://github.com/bitsstdcheee/code-backup-status/actions/workflows/runner.yml/badge.svg)](https://github.com/bitsstdcheee/code-backup-status/actions/workflows/runner.yml)");

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

// ojProblemSetPrefix 用于存储每个OJ的题目集的 url 前缀, bool 存储是否为纯数字
std::map<string, pair<string, bool> > ojProblemSetPrefix;

// ojHomeUrl 用户存储每个OJ的主页 url
// pair<name: string, url: string> 
std::map<string, pair<string, string> > ojHomeUrl;

// initOjProblemSetPrefix 用于初始化 ojProblemSetPrefix
void initOjProblemSetPrefix() {
    auto& op = ojProblemSetPrefix;
    op["Luogu"] = make_pair("https://www.luogu.com.cn/problem/", false);
    // op["BZOJ"]  // BZOJ 目前找不到相应评测网站
    op["HDU"] = make_pair("https://acm.hdu.edu.cn/showproblem.php?pid=", true);
    op["LibreOJ"] = make_pair("https://loj.ac/p/", true);
    op["POJ"] = make_pair("http://poj.org/problem?id=", true);
    op["SPOJ"] = make_pair("https://www.spoj.com/problems/", false); // SPOJ 题目 id 为大写英文字母的组合
    op["MagicOJ"] = make_pair("http://www.magicoj.com/p/", true);
}

void initOjHomeUrl() {
    auto& op = ojHomeUrl;
    op["Luogu"] = make_pair("洛谷", "https://www.luogu.com.cn/");
    op["HDU"] = make_pair("HDU", "https://acm.hdu.edu.cn/");
    op["LibreOJ"] = make_pair("LOJ", "https://loj.ac/");
    op["POJ"] = make_pair("POJ", "http://poj.org/");
    op["SPOJ"] = make_pair("SPOJ", "https://www.spoj.com/");
    op["MagicOJ"] = make_pair("MagicOJ", "http://www.magicoj.com/");
    op["Codeforces"] = make_pair("CF", "https://codeforces.com/");
}

// PureNumber: 将 id 中的纯数字提取出来
string PureNumber(string id) {
    string res = "";
    for (auto& ch : id) {
        if (isdigit(ch)) res += ch;
    }
    return res;
}

// ProblemUrlCheck 用于判断是否存在某个 OJ 的题目集 url
bool ProblemUrlCheck(string oj) {
    auto& op = ojProblemSetPrefix;
    return !(op.find(oj) == op.end());
}

// ProblemUrl 用于输出某个 OJ 的题目集 url
string ProblemUrl(string oj, string id) {
    if (!ProblemUrlCheck(oj)) return "";
    auto& op = ojProblemSetPrefix;
    if (op[oj].second) return op[oj].first + PureNumber(id);
    else return op[oj].first + id;
}

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

// 获取不含点的文件拓展名
string getFileExtensionWithoutDot(const string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos && dotPos < filename.size() - 1) {
        return filename.substr(dotPos + 1);
    }
    // 如果没有找到点，表示没有扩展名
    return "";
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
    std::string pattern = R"(^(.*?)_ver\.(.*?)_(.*?)(?:_((?:[^\d_]+(?:_[^\d_]+)*)))?_?((\d+)pt)?((?:_(\d+))*)?(\.cpp|\.rs|\.in|\.out|\.ans)$)";
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

// 判断文件是否是 C++ 源代码文件
bool isCppFile(const string& filename) {
    return filename.size() > 4 && filename.substr(filename.size() - 4) == ".cpp";
}

// 判断文件是否是 Rust 源代码文件
bool isRustFile(const string& filename) {
    return filename.size() > 3 && filename.substr(filename.size() - 3) == ".rs";
}

// 判断文件是否是源代码文件
bool isCodeFile(const string& filename) {
    return isCppFile(filename) || isRustFile(filename);
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

// AtCoder 转洛谷 ID 时部分需要特殊处理的比赛 ID
// 若使用 crawl 获取, 则基本不需要特殊处理
const pair<string, string> AtCoderContestIdRewrite[] = {
    {"icpc2015summerday2", "icpc2015summer_day2"}
};

// 获取洛谷下 AtCoder 题号名称
// eg. abc303d -> AT_abc303_d
string getAtCoderLuoguFormatted(const string& id) {
    // ID 最后一位为题面编号 (字母), 其余为比赛编号
    // 转为小写
    string id_lower = "";
    for (char c : id) {
        if (c >= 'A' && c <= 'Z')
            id_lower += c - 'A' + 'a';
        else if (c == '-')
            id_lower += '_';
        else
            id_lower += c;
    }
    char question_id = id_lower[id_lower.size() - 1];
    string contest_id = id_lower.substr(0, id_lower.size() - 1);
    // 匹配重写规则
    for (pair<string, string> rule : AtCoderContestIdRewrite) {
        if (contest_id == rule.first) {
            contest_id = rule.second;
            break;
        }
    }
    string luogu_id = contest_id + "_" + question_id;
    return luogu_id;
}

string parseCodeForcesUrl(const string& id) {
    char question_id = id[id.size() - 1];
    string contest_id = (id[0] == 'P' ? 
        id.substr(1, id.size() - 2) :
        id.substr(0, id.size() - 1));
    return "https://codeforces.com/problemset/problem/" + contest_id 
        + "/" + question_id; 
}

const string BadgeSchema = "https";
const string BadgeHost = "img.shields.io";
const string BadgeRoute = "/badge/";

string getUrlEncode(string s) {
    string res = "";
    for (char c : s) {
        if (c == ':') res += "%3A";
        else if (c == '/') res += "%2F";
        else if (c == '?') res += "%3F";
        else if (c == '#') res += "%23";
        else if (c == '[') res += "%5B";
        else if (c == ']') res += "%5D";
        else if (c == '@') res += "%40";
        else if (c == '!') res += "%21";
        else if (c == '$') res += "%24";
        else if (c == '&') res += "%26";
        else if (c == '\'') res += "%27";
        else if (c == '(') res += "%28";
        else if (c == ')') res += "%29";
        else if (c == '*') res += "%2A";
        else if (c == '+') res += "%2B";
        else if (c == ',') res += "%2C";
        else if (c == ';') res += "%3B";
        else if (c == '=') res += "%3D";
        else if (c == '%') res += "%25";
        else if (c == ' ') res += "%20";

        else if (c == '-') res += '_'; // 不属于 UrlEncode, 防止 API 传参时出错

        else {
            res += c;
        }
    }
    return res;
}

string getBadgeUrl(string left, string right = "", string color = "") {
    string url = BadgeSchema + "://" + BadgeHost + BadgeRoute;
    left = getUrlEncode(left);
    right = getUrlEncode(right);
    url += left;
    if (right != "") url += "-" + right;
    if (color != "") url += "-" + color;
    return url;
}

string generate_html_photo_with_href(const string& photo, const string& href, const string& alt = "") {
    return "<a href=\"" + href + "\"" + (alt == "" ? "" : " alt=\"" + alt + "\"") + "><img src=\"" + photo + "\" /></a>";
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
            else if (isCodeFile(filename)) {
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
    #ifdef OUT_HTML_Markdown
    cout << "<tbody>\n";
    #endif
    for (const auto& oj : oj_map) {
        for (const auto& problem : oj.second) {
            #ifdef OUT_HTML_Markdown
            cout << "<tr>\n";
            #endif
            string id = problem.first;
            vector<pair<FileName, string>> cpp_files;
            string status = "";
            bool has_rust = false;
            string current_oj = oj.first;
            int max_pt = -1;
            int count = 0;
            vector<string> data_points;
            vector<string> problem_second = problem.second;
            sort(problem_second.begin(), problem_second.end());
            for (const auto& filename : problem_second) {
                if (isDataFile(filename)) {
                    size_t pos = filename.find_last_of('_');
                    if (pos != string::npos) {
                        data_points.push_back(filename.substr(pos + 1));
                    }
                }
                else if (isCodeFile(filename)) {
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
                    if (!has_rust && fn.status == "AC" && getFileExtensionWithoutDot(filename) == "rs") {
                        has_rust = true;
                    }
                }
            }
            #ifdef OUT_Markdown
            #ifdef OUT_ProblemUrl
            if (ProblemUrlCheck(current_oj)) {
                #ifdef OUT_HTML_Markdown
                cout << "<td><a href=\"" + ProblemUrl(current_oj, id) + "\">" << current_oj << "</a></td>";
                #else
                cout << "[" << current_oj << "](" << ProblemUrl(current_oj, id) << ") | " ;
                #endif
            } else if (current_oj == "Codeforces") {
                // CF 特殊处理
                #ifdef OUT_HTML_Markdown
                cout << "<td><a href=\"" + parseCodeForcesUrl(id) + "\">" << current_oj << "</a></td>";
                #else
                cout << "[" << current_oj << "](" << parseCodeForcesUrl(id) << ") | ";
                #endif
            } else {
                // 当前 oj 不支持 url, 回退到原来格式
                #ifdef OUT_HTML_Markdown
                cout << "<td>" << current_oj << "</td>";
                #else
                cout << current_oj << " | " ;
                #endif
            }
            #else
            cout << current_oj << " | " ;
            #endif
            #ifdef OUT_HTML_Markdown
            cout << "<td>";
            #endif
            #ifdef OUT_ColorAC
            cout << "$\\textcolor{" ;
            if (status == "AC") {
                if (has_rust) cout << ColorRustRGB;
                else cout << ColorAcceptedRGB;
            }
            else if (status == "Waiting") cout << ColorDefaultRGB;  // Waiting 状态时 color 留空以保持默认颜色
            else cout << ColorUnacceptedRGB;
            cout << "}{\\text{" << KatexFormat(id, 
            #ifdef OUT_DoubleBackslash
            true
            #else
            false
            #endif
            );
            #ifdef OUT_HTML_Markdown
            cout << "}}$</td><td>" << count << "</td>";
            #else
            #ifdef OUT_HTML_Markdown
            cout << "}}$</td><td>" << count << "</td>";
            #else
            cout << "}}$ | " << count << " | ";
            #endif
            #endif
            #else
            #ifdef OUT_HTML_Markdown
            cout << id << (has_rust ? " <img src=\"https://img.shields.io/badge/Rust-DEA584\">" : "") << "</td><td>" << count << "</td>";
            #else
            cout << id << " | " << count << " | ";
            #endif
            #endif
            #else
            cout << "#" << oj.first << "," << id << "," << count << ",";
            #endif
            #ifdef OUT_HTML_Markdown
            cout << "<td>";
            #endif
            if (status.empty()) {
                #ifdef OUT_Markdown
                #ifdef OUT_HTML_Markdown
                cout << "N/A</td>";
                #else
                cout << "N/A | ";
                #endif
                #else
                cout << "N/A,";
                #endif
            }
            else {
                if (status == "AC") {
                    #ifdef OUT_Markdown
                    #ifdef OUT_HTML_Markdown
                    cout << "AC</td>";
                    #else
                    cout << "AC | ";
                    #endif
                    #else
                    cout << "AC,";
                    #endif
                }
                else {
                    #ifdef OUT_Markdown
                    #ifdef OUT_HTML_Markdown
                    cout << status << "</td>";
                    #else
                    cout << status << " | ";
                    #endif
                    #else
                    cout << status << ",";
                    #endif
                }
            }
            #ifdef OUT_HTML_Markdown
            cout << "<td>";
            #endif
            if (max_pt < 0) {
                // max_pt 默认值为 -1
                #ifdef OUT_Markdown
                #ifdef OUT_HTML_Markdown
                cout << "N/A</td>";
                #else
                cout << "N/A | ";
                #endif
                #else
                cout << "N/A,";
                #endif
            }
            else {
                #ifdef OUT_Markdown
                #ifdef OUT_HTML_Markdown
                cout << max_pt << "</td>";
                #else
                cout << max_pt << " | ";
                #endif
                #else
                cout << max_pt << ",";
                #endif
            }
            #ifdef OUT_Markdown
            #ifdef OUT_HTML_Markdown
            cout << "<td>" << data_points.size() << "</td>";
            #else
            cout << data_points.size() << " | ";
            #endif
            #else
            cout << data_points.size() << endl;
            cout << "  cpp files: ";
            #endif
            #ifdef OUT_Markdown
            bool first_out = true; // 保证最后没有多余的逗号
            #endif
            #ifdef OUT_HTML_Markdown
            cout << "<td>";
            #endif
            for (const auto& cpp_file : cpp_files) {
                #ifdef OUT_Markdown
                #ifdef OUT_Badge
                string exName = getFileExtensionWithoutDot(cpp_file.second);
                if (first_out)
                    cout << generate_html_photo_with_href(getBadgeUrl(cpp_file.first.date, cpp_file.first.status + (exName == "rs" ? " (Rust)" : ""), cpp_file.first.status.substr(0, 2) == "AC" ? (exName == "rs" ? "DEA584" : "52C41A") : "E74C3C"), repo_prefix + cpp_file.second);
                else cout << "<br>" << generate_html_photo_with_href(getBadgeUrl(cpp_file.first.date, cpp_file.first.status + (exName == "rs" ? " (Rust)" : ""), cpp_file.first.status.substr(0, 2) == "AC" ? (exName == "rs" ? "DEA584" : "52C41A") : "E74C3C"), repo_prefix + cpp_file.second);
                #else
                if (first_out) cout << "[" << getFileDescription(cpp_file.first, true) << (getFileExtensionWithoutDot(cpp_file.second) == "rs" ? "(**Rust**)" : "") << "](" << repo_prefix << cpp_file.second << ")";
                else cout << "<br>[" << getFileDescription(cpp_file.first, true) << (getFileExtensionWithoutDot(cpp_file.second) == "rs" ? "(**Rust**)" : "") << "](" << repo_prefix << cpp_file.second << ")";
                #endif
                first_out = false;
                #else
                cout << cpp_file.second << " ";
                #endif
            }
            #ifdef OUT_Markdown
            #ifdef OUT_HTML_Markdown
            cout << "</td>";
            #else
            cout << " | ";
            #endif
            #else
            cout << endl;
            cout << "  data points: ";
            #endif
            #ifdef OUT_Markdown
            first_out = true;
            #endif
            #ifdef OUT_HTML_Markdown
            cout << "<td>";
            #endif
            for (const auto& data_point : data_points) {
                #ifdef OUT_Markdown
                #ifdef OUT_HTML_Markdown
                if (first_out)
                cout << "<a href=\"" << repo_prefix << current_oj << "/" << id << "_" << data_point << "\">" << data_point << "</a>";
                else cout << "<br><a href=\"" << repo_prefix << current_oj << "/" << id << "_" << data_point << "\">" << data_point << "</a>";
                first_out = false;
                #else
                if (first_out) cout << "[" << data_point << "](" << repo_prefix << current_oj << "/" << id << "_" << data_point << ")";
                else cout << "<br>[" << data_point << "](" << repo_prefix << current_oj << "/" << id << "_" << data_point << ")";
                first_out = false;
                #endif
                #else
                cout << data_point << " ";
                #endif
            }
            #ifdef OUT_HTML_Markdown
            cout << "</td>";
            #endif
            #ifdef OUT_Markdown
            #ifdef OUT_Checkbox
            cout << " | " << (status == "AC" ? "<ul><li>[x] 完成</li></ul>" : "<ul><li>[ ] 未完成</li></ul>");
            #endif
            #endif
            cout << endl;
            #ifdef OUT_HTML_Markdown
            cout << "</tr>\n";
            #endif
        }
    }
    #ifdef OUT_HTML_Markdown
    cout << "</tbody>\n";
    #endif
}

void outputOjUrl() {
#ifndef OUT_Markdown
    return;
#else
    bool first = true;
    for (auto oj_pair: ojHomeUrl) {
        if (first) first = false;
        else cout << " \\| ";
        auto& oj = oj_pair.second;
        cout << "[" << oj.first << "](" << oj.second << ")";
    }
    cout << endl;
#endif
}

void processAtCoder(string path, string folderName = "Atcoder") {
#ifndef OUT_Markdown
    cout << "Warning: AtCoder 处理器不支持非 Markdown 环境下的输出" << endl;
    return;
#endif
    // 单独处理 AtCoder 代码文件
    #ifdef OUT_HTML_Markdown
    cout << "### AtCoder" << endl << endl;
    cout << "<table>\n";
    cout << "<thead>\n<tr>\n<th>洛谷</th>\n<th>原题</th>\n<th>编号</th>\n<th>提交次数</th>\n<th>最终提交状态</th>\n<th>最高分数</th>\n<th>代码提交文件</th>\n</tr></thead>\n";
    #else
    cout << "### AtCoder" << endl;
    cout << "洛谷 | 原题 | 编号 | 提交次数 | 最终提交状态 | 最高分数 | 代码提交文件";
    cout << endl;
    cout << "---- | --- | --- | -------- | ----------- | ------- | ---- |" << endl;
    #endif
    
    path = (filesystem::path(path) / filesystem::path(folderName)).string();
    map<string, vector<string>> fileList;
    fileList.clear();
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            const string& current_file_name = entry.path().filename().string();
            if (!isCodeFile(current_file_name)) continue;
            FileName fn = parseFileName(current_file_name);
            fileList[fn.id].push_back(current_file_name);
        }
    }
    #ifdef OUT_HTML_Markdown
    cout << "<tbody>\n";
    #endif
    
    for (const auto& problem : fileList) {
        #ifdef OUT_HTML_Markdown
        cout << "<tr>\n";
        #endif
        string id = problem.first;
        vector<pair<FileName, string>> cpp_files;
        string status = "";
        string current_oj = "AtCoder";
        int max_pt = -1;
        int count = 0;
        vector<string> data_points;
        vector<string> problem_second = problem.second;
        sort(problem_second.begin(), problem_second.end());
        for (const auto& filename : problem_second) {
            if (isDataFile(filename)) {
                size_t pos = filename.find_last_of('_');
                if (pos != string::npos) {
                    data_points.push_back(filename.substr(pos + 1));
                }
            }
            else if (isCodeFile(filename)) {
                FileName fn = parseFileName(filename);
                if (fn.id == id) {
                    cpp_files.push_back(make_pair(fn, folderName + "/" + filename));
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
        const string atCoderLuoguFormattedId = getAtCoderLuoguFormatted(id);
        #ifdef OUT_HTML_Markdown
        cout << "<td><a href=\"" << "https://www.luogu.com.cn/problem/AT_" + atCoderLuoguFormattedId << "\">洛谷</a></td>"; 
        cout << "<td><a href=\"" << "https://www.luogu.com.cn/remoteJudgeRedirect/atcoder/" + atCoderLuoguFormattedId << "\">Link</a></td>";
        #else
        cout << "[洛谷](https://www.luogu.com.cn/problem/AT_" + atCoderLuoguFormattedId + ") | ";
        cout << "[Link](https://www.luogu.com.cn/remoteJudgeRedirect/atcoder/" + atCoderLuoguFormattedId + ") | ";
        #endif
        #ifdef OUT_HTML_Markdown
        cout << "<td>";
        #endif
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
        );
        #ifdef OUT_HTML_Markdown
        cout << "}}$</td><td>" << count << "</td>";
        #else
        cout << "}}$ | " << count << " | ";
        #endif
        #else
        #ifdef OUT_HTML_Markdown
        cout << id << "</td><td>" << count << "</td>";
        #else
        cout << id << " | " << count << " | ";
        #endif
        #endif
        #else
        cout << "#" << folderName << "," << id << "," << count << ",";
        #endif
        #ifdef OUT_HTML_Markdown
        cout << "<td>";
        #endif
        if (status.empty()) {
            #ifdef OUT_Markdown
            #ifdef OUT_HTML_Markdown
            cout << "N/A</td>";
            #else
            cout << "N/A | ";
            #endif
            #else
            cout << "N/A,";
            #endif
        }
        else {
            if (status == "AC") {
                #ifdef OUT_Markdown
                #ifdef OUT_HTML_Markdown
                cout << "AC</td>";
                #else
                cout << "AC | ";
                #endif
                #else
                cout << "AC,";
                #endif
            }
            else {
                #ifdef OUT_Markdown
                #ifdef OUT_HTML_Markdown
                cout << status << "</td>";
                #else
                cout << status << " | ";
                #endif
                #else
                cout << status << ",";
                #endif
            }
        }
        #ifdef OUT_HTML_Markdown
        cout << "<td>";
        #endif
        if (max_pt < 0) {
            // max_pt 默认值为 -1
            #ifdef OUT_Markdown
            #ifdef OUT_HTML_Markdown
            cout << "N/A</td>";
            #else
            cout << "N/A | ";
            #endif
            #else
            cout << "N/A,";
            #endif
        }
        else {
            #ifdef OUT_Markdown
            #ifdef OUT_HTML_Markdown
            cout << max_pt << "</td>";
            #else
            cout << max_pt << " | ";
            #endif
            #else
            cout << max_pt << ",";
            #endif
        }
        #ifdef OUT_Markdown
        bool first_out = true; // 保证最后没有多余的逗号
        #endif
        #ifdef OUT_HTML_Markdown
        cout << "<td>";
        #endif
        for (const auto& cpp_file : cpp_files) {
            #ifdef OUT_Markdown
            #ifdef OUT_Badge
            string exName = getFileExtensionWithoutDot(cpp_file.second);
            if (first_out) 
                cout << generate_html_photo_with_href(getBadgeUrl(cpp_file.first.date, cpp_file.first.status + (exName == "rs" ? " (Rust)" : ""), cpp_file.first.status.substr(0, 2) == "AC" ? (exName == "rs" ? "DEA584" : "52C41A") : "E74C3C"), repo_prefix + cpp_file.second);
            else cout << "<br>" << generate_html_photo_with_href(getBadgeUrl(cpp_file.first.date, cpp_file.first.status + (exName == "rs" ? " (Rust)" : ""), cpp_file.first.status.substr(0, 2) == "AC" ? (exName == "rs" ? "DEA584" : "52C41A") : "E74C3C"), repo_prefix + cpp_file.second);
            #else
            if (first_out) cout << "[" << getFileDescription(cpp_file.first, true) << (getFileExtensionWithoutDot(cpp_file.second) == "rs" ? " (**Rust**)" : "") <<"](" << repo_prefix << cpp_file.second << ")";
            else cout << "<br>[" << getFileDescription(cpp_file.first, true) << (getFileExtensionWithoutDot(cpp_file.second) == "rs" ? "(**Rust**)" : "") << "](" << repo_prefix << cpp_file.second << ")";
            #endif
            first_out = false;
            #else
            cout << cpp_file.second << " ";
            #endif
        }
        #ifdef OUT_Markdown
        #ifdef OUT_HTML_Markdown
        cout << "</td>";
        #endif
        #ifdef OUT_Checkbox
        cout << " | " << (status == "AC" ? "<ul><li>[x] 完成</li></ul>" : "<ul><li>[ ] 未完成</li></ul>");
        #endif
        #endif
        cout << endl;
        #ifdef OUT_HTML_Markdown
        cout << "</tr>\n";
        #endif
    }
    
    #ifdef OUT_HTML_Markdown
    cout << "</tbody>\n";
    #endif
}

int main() {
#ifdef OUT_ProblemUrl
initOjProblemSetPrefix();
#endif
initOjHomeUrl();
#ifdef OUT_Markdown
    cout << markdown_pre << endl;
    cout << endl;
    outputOjUrl();
    #ifdef OUT_HTML_Markdown
    cout << "<table>\n";
    cout << "<thead>\n<tr>\n<th>OJ</th>\n<th>ID</th>\n<th>提交次数</th>\n<th>最终提交状态</th>\n<th>最高分数</th>\n<th>数据点数量</th>\n<th>代码提交文件</th>\n<th>数据点文件</th>\n</tr></thead>\n";
    #else
    // 输出表头
    cout << "OJ | ID | 提交次数 | 最终提交状态 | 最高分数 | 数据点数量 | 代码提交文件 | 数据点文件";
    cout << endl;
    cout << "-- | -- | ------- | ------------ | ------ | --------- | ------------ | ---------";
    cout << endl;
    #endif
#endif
#ifndef CI
    // processDirectory("D:\\code-backup\\");
    processDirectory("F:\\code-backup\\");
#else
    processDirectory("code-backup");
#endif
#ifdef OUT_HTML_Markdown
    cout << "</table>";
    cout << endl << endl; // 分隔 HTML 代码和 Markdown
#endif
    cout << endl;
#ifndef CI
    processAtCoder("F:\\code-backup\\");
#else
    processAtCoder("code-backup");
#endif
#ifdef OUT_HTML_Markdown
    cout << "</table>";
    cout << endl << endl;
#endif
    return 0;
}
