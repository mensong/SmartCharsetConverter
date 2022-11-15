# 智能编码集转换器
本程序用于自动识别文件夹下所有文本文件，自动识别原编码（不必担心反复转码出错了），批量转换到UTF-8等字符集。

功能：
* 批量转UTF-8/UTF-8-sig/GB18030等
* 批量转CRLF/LF/CR换行符

运行要求：
* Win10 x64
* Win7 x64（理论上可以，没尝试）

![img](snapshot/v0.2.png "截图")

# 特别优点
字符集探测是著名的老大难问题，就是说，怎样在不知道字符编码的情况下，探测出文本是什么编码，什么字符集。乱码也由此产生。

我在对比了诸多字符集探测库之后，选定了Notepad3使用的魔改版uchardet，这个魔改版uchardet经过Notepad3作者精心调教，精度比原版uchardet更高！

# 版本记录
v0.1 实现基本功能：可以探测字符集，转换字符集
v0.2 增加windows-1252支持。文件现在可以选择“不过滤”和“智能识别”。
latest “添加文件夹”现在可以记住上一次选的路径了。

# TODO
* 转换前检查是否会丢失字符。
* 转换前再次检查一次字符集，已免出现加载后用户更改了字符集后转换出错的情况。
* 右键菜单加入“转换到xxx编码”，以实现单个/多个文件手动转码。
* 增加一个刷新按钮。
* 使用ini文件保存配置。
* 增加更多编码集（UTF-16LE/UTF-16BE/BIG5等）。

# Reference

[WTL](https://sourceforge.net/projects/wtl)

[uchardet](https://github.com/freedesktop/uchardet)
