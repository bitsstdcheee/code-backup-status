# coding:utf8
import requests
import json
import sys
import locale
from bs4 import BeautifulSoup
import re
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
        res = json.load(f)
    msg = res['message']
    code = res['code']
    if res == str_not_signed:
        print("用户尚未登录")
        exit(2)
    if res == str_have_punched:
        print("用户已打卡")
        exit(0)
    if code == 200:
        # 未打卡且打卡成功
        print("打卡请求正常返回")
        content = res['more']['html']
        soup = BeautifulSoup(content, 'html.parser')
        print(soup.getText())
        pattern = re.compile(R'(bitsstdcheee 的运势)(?P<status>§.+§)[\n\s]+(?P<first>(?:\S[\S\x20]+\n)+)[\n\s]+(?P<second>(?:\S[\S\x20]+\n)+)[\n\s]+你已经在洛谷连续打卡了 (?P<day>\S+) 天')
        result = pattern.match(soup.getText())
        # print(result.groupdict())
        reg = result.groupdict()
        utc_dt = datetime.utcnow()
        dt = utc_dt.astimezone(timezone(timedelta(hours=8))) # UTF+8
        dfs = dt.strftime('%Y/%m/%d')
        out = f'\n\n{dfs}\nbitsstdcheee 的运势\n{reg["status"]}\n{reg["first"]}{reg["second"]}\n'
        print(out)
        print("尝试写入 lucky.txt")
        with open('lucky.txt', 'a+', encoding='utf-8') as luckyf:
            luckyf.write(out)
        with open('lucky.txt', 'r', encoding='utf-8') as f1:
            ld = f1.read()
            pattern1 = re.compile(R'\n{3,}')
            result1 = pattern1.sub('\n\n', ld)
            pattern2 = re.compile(R'\n{2,}$')
            result2 = pattern2.sub('', result1)
        with open('lucky.txt', 'w', encoding='utf-8') as luckyout:
            luckyout.write(result2)
        exit(0)
        

    # print(res)