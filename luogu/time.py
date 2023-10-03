import datetime
import pytz

# 获取当前时间
current_time = datetime.datetime.now()

# 创建UTC+8的时区对象
utc_plus_8 = pytz.timezone('Asia/Shanghai')

# 将当前时间转换为UTC+8时间
utc_plus_8_time = current_time.astimezone(utc_plus_8)

formatted_time = utc_plus_8_time.strftime("%Y-%m-%d %H:%M:%S")

# 输出UTC+8时间
print("### Action 运行时间 (UTC+8):", formatted_time)