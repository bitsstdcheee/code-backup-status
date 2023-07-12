# coding:utf8
import requests
import json
import sys
import locale
from bs4 import BeautifulSoup
import re
import traceback
from datetime import datetime, timezone, timedelta

str_not_signed = '请登录洛谷进行打卡，获得今日运势。'
str_have_punched = '今天你已经打过卡了哦，要一步一个脚印，不能急于求成！'

def cookie_from_file():
    with open('./cookie.txt', 'r', encoding='utf-8') as f:
        data = f.read().splitlines()
        if len(data) < 1:
            return ''
        return data[0]

def punch(cookie):
    res = requests.get('https://www.luogu.com.cn/index/ajax_punch', headers={
        "Host""Host": "www.luogu.com.cn",
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36 Edg/114.0.1823.51",
        "Sec-Ch-Ua": '"Not.A/Brand";v="8", "Chromium";v="114", "Microsoft Edge";v="114"',
        "Sec-Ch-Ua-Mobile": "?0",
        "Sec-Ch-Ua-Platform": "Windows",
        "Accept": "*/*",
        "Accept-Language": "zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2",
        "Accept-Encoding": "gzip, deflate",
        "Connection": "keep-alive",
        "Referer": "https://www.luogu.com.cn/",
        "Cache-Control": "no-cache",
        "TE": "Trailers",
        "Cookie": cookie
    })
    dt = res.text
    # print(dt)
    dt = dt.encode('utf-8').decode('unicode-escape')
    print(dt)
    with open('json.txt', 'w', encoding='utf-8') as f:
        f.write(dt)

def lucky_format(txt: str) -> str:
    pattern = r"(万事皆宜|诸事不宜|宜(:|：)|忌(:|：))"
    result = re.sub(pattern, r"\n\g<0>", txt)
    # print(result)
    return result

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("请输入参数")
        exit(1)
    if sys.argv[1] == 'file':
        in_cookie = cookie_from_file()
        print("将从文本中读取 cookie")
    else:
        in_cookie = sys.argv[1]
    punch(cookie=in_cookie)
    with open('json.txt', 'r', encoding='utf-8') as f:
        lines = f.readlines()
        res = ""
        for i in lines:
            res = res + i
        res = res.replace('\n', '')
        # res = res.replace(' ', '')
        # print(res)
        pat = re.compile(r'"more":{"html":"(.+)"}}$')
        result1 = pat.findall(res)
        # print(result1)
        if len(result1) >= 1:
            html_txt = result1[0]
            html_txt = html_txt.replace('"', r'\"')
            # print(html_txt)
            res = pat.sub(r'"more":{"html":"' + html_txt + r'"}}', res)
            # print(res)
    js = json.loads(res)
    if 'more' in js and 'html' in js['more']:
        soup = BeautifulSoup(js['more']['html'], 'html.parser')
        text = soup.getText(strip=True)
        # print(text + "|||end")
        pattern = re.compile(R'(\S+)\s{0,}的运势\s{0,}(§.+§)\s{0,}(\S[\s\S]+\S)\s{0,}你已经在洛谷连续打卡了.+$')
        # pattern = re.compile(R'(\S+)\s{0,}的运势.+$')
        result = pattern.match(text)
        # print(result)
        try:
            user_id = result[1]
            lucky_text = result[2]
            lucky_detail = result[3]
            lucky_detail_formatted = lucky_format(lucky_detail)
            utc_dt = datetime.utcnow()
            dt = utc_dt.astimezone(timezone(timedelta(hours=8))) # UTF+8
            dfs = dt.strftime('%Y/%m/%d')
            out_summary = f'{dfs}\n{user_id} 的运势\n{lucky_text}{lucky_detail_formatted}\n'
            out = f'\n\n{out_summary}'
            print(out)
            print("尝试写入 lucky.txt")
            with open('lucky.txt', 'a+', encoding='utf-8') as luckyf:
                luckyf.write(out)
            print("尝试写入 summary.txt")
            with open('summary.txt', 'w', encoding='utf-8') as sf:
                sf.write(out_summary)
            with open('lucky.txt', 'r', encoding='utf-8') as f1:
                ld = f1.read()
                pattern1 = re.compile(R'\n{3,}')
                result1 = pattern1.sub('\n\n', ld)
                pattern2 = re.compile(R'\n{2,}$')
                result2 = pattern2.sub('', result1)
            with open('lucky.txt', 'w', encoding='utf-8') as luckyout:
                luckyout.write(result2)
        except Exception as e:
            print("遇到未知错误:", e)
            print(traceback.format_exc())
            exit(3)
        exit(0)
        print(out)
    if res.find(str_not_signed) != -1:
        print("用户尚未登录")
        exit(2)
    if res.find(str_have_punched) != -1:
        print("用户已打卡")
        exit(1)
    if res.find('{"code":200') != -1:
        # 未打卡且打卡成功
        print("打卡请求正常返回")
        print("返回信息未处理")
        exit(0)
    else:
        exit(3)

    # print(res)