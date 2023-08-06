#include "DialogMain.h"

#include <tstring.h>
#include <FileFunction.h>
#include "Control/TMenu.h"

#ifdef _DEBUG
#include <iostream>
#include <cassert>
#endif

#include <stdexcept>
#include <sstream>
#include <set>
#include <regex>

#undef min
#undef max

const std::tstring appTitle = TEXT("智能编码集转换器 v0.41 by Tom Willow");

using namespace std;

DialogMain::DialogMain() {}

DialogMain::~DialogMain() {}

void DialogMain::OnClose() {
    if (thRunning) {
        doCancel = true;
        fu.get();
    }
    EndDialog(0);
}

BOOL DialogMain::OnInitDialog(CWindow wndFocus, LPARAM lInitParam) {
    // 设置窗口的大小图标
    // 大图标：按下alt+tab键切换窗口时对应的图标
    // 小图标：就是窗口左上角对应的那个图标
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    ::SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    ::SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    SetWindowText(appTitle.c_str());

    BOOL bHandle = true;

    CoreInitOption coreOpt;
    coreOpt.fnUIAddItem = [this](std::wstring filename, std::wstring fileSizeStr, std::wstring charsetStr,
                                 std::wstring lineBreakStr, std::wstring textPiece) {
        PostUIFunc([=]() {
            auto count = listview.GetItemCount();
            listview.AddItem(count, static_cast<int>(ListViewColumn::INDEX), to_tstring(count + 1).c_str());
            listview.AddItem(count, static_cast<int>(ListViewColumn::FILENAME), filename.c_str());
            listview.AddItem(count, static_cast<int>(ListViewColumn::FILESIZE), fileSizeStr.c_str());
            listview.AddItem(count, static_cast<int>(ListViewColumn::ENCODING), charsetStr.c_str());
            listview.AddItem(count, static_cast<int>(ListViewColumn::LINE_BREAK), lineBreakStr.c_str());
            listview.AddItem(count, static_cast<int>(ListViewColumn::TEXT_PIECE), textPiece.c_str());

            // listview滚动到最下面
            listview.SelectItem(count);
        });
    };
    core = make_unique<Core>(TEXT("SmartCharsetConverter.ini"), coreOpt);

    // 包含/排除指定后缀
    SetFilterMode(core->GetConfig().filterMode);
    // GetDlgItem(IDC_EDIT_INCLUDE_TEXT).SetWindowTextW(core->GetConfig().includeRule);

    // target
    SetOutputTarget(core->GetConfig().outputTarget);
    GetDlgItem(IDC_EDIT_OUTPUT_DIR).SetWindowTextW(core->GetConfig().outputDir.c_str());
    static_cast<CEdit>(GetDlgItem(IDC_EDIT_OUTPUT_DIR)).SetReadOnly(true);

    SetOutputCharset(core->GetConfig().outputCharset);

    // enable/disable line breaks
    CButton(GetDlgItem(IDC_CHECK_CONVERT_RETURN)).SetCheck(core->GetConfig().enableConvertLineBreaks);
    OnBnClickedCheckConvertReturn(0, 0, 0, bHandle);
    CButton(GetDlgItem(IDC_RADIO_CRLF + static_cast<int>(core->GetConfig().lineBreak))).SetCheck(true);

    // listview
    listview.SubclassWindow(GetDlgItem(IDC_LISTVIEW)); // 必须用SubclassWindow传入句柄，才能让MSG_MAP生效

    listview.ModifyStyle(0, LVS_REPORT);
    listview.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    listview.AddColumn(TEXT("序号"), static_cast<int>(ListViewColumn::INDEX));
    listview.SetColumnWidth(0, 40);

    listview.AddColumn(TEXT("文件名"), static_cast<int>(ListViewColumn::FILENAME));
    listview.SetColumnWidth(1, 300);

    listview.AddColumn(TEXT("大小"), static_cast<int>(ListViewColumn::FILESIZE));
    listview.SetColumnWidth(2, 60);

    listview.AddColumn(TEXT("编码"), static_cast<int>(ListViewColumn::ENCODING));
    listview.SetColumnWidth(3, 60);

    listview.AddColumn(TEXT("换行符"), static_cast<int>(ListViewColumn::LINE_BREAK));
    listview.SetColumnWidth(4, 60);

    listview.AddColumn(TEXT("文本片段"), static_cast<int>(ListViewColumn::TEXT_PIECE));
    listview.SetColumnWidth(5, 200);

    // 启用拖放
    ::DragAcceptFiles(listview, true);

    setlocale(LC_CTYPE, "");

    CenterWindow();

    return 0;
}

void DialogMain::SetFilterMode(Configuration::FilterMode mode) {
    core->SetFilterMode(mode);

    CButton(GetDlgItem(IDC_RADIO_STRETEGY_NO_FILTER)).SetCheck(false);
    CButton(GetDlgItem(IDC_RADIO_STRETEGY_SMART)).SetCheck(false);
    CButton(GetDlgItem(IDC_RADIO_STRETEGY_MANUAL)).SetCheck(false);
    switch (mode) {
    case Configuration::FilterMode::NO_FILTER:
        CButton(GetDlgItem(IDC_RADIO_STRETEGY_NO_FILTER)).SetCheck(true);
        break;
    case Configuration::FilterMode::SMART:
        CButton(GetDlgItem(IDC_RADIO_STRETEGY_SMART)).SetCheck(true);
        break;
    case Configuration::FilterMode::ONLY_SOME_EXTANT:
        CButton(GetDlgItem(IDC_RADIO_STRETEGY_MANUAL)).SetCheck(true);
        break;
    default:
        assert(0);
    }

    GetDlgItem(IDC_EDIT_INCLUDE_TEXT).EnableWindow(mode == Configuration::FilterMode::ONLY_SOME_EXTANT);
}

void DialogMain::SetOutputTarget(Configuration::OutputTarget outputTarget) {
    core->SetOutputTarget(outputTarget);
    bool isToOrigin = (outputTarget == Configuration::OutputTarget::ORIGIN);

    CButton(GetDlgItem(IDC_RADIO_TO_ORIGIN)).SetCheck(isToOrigin);
    CButton(GetDlgItem(IDC_RADIO_TO_DIR)).SetCheck(!isToOrigin);

    GetDlgItem(IDC_EDIT_OUTPUT_DIR).EnableWindow(!isToOrigin);
    GetDlgItem(IDC_BUTTON_SET_OUTPUT_DIR).EnableWindow(!isToOrigin);
}

void DialogMain::SetOutputCharset(CharsetCode charset) {
    core->SetOutputCharset(charset);
    bool isNormalCharset = Configuration::IsNormalCharset(charset);

    CButton(GetDlgItem(IDC_RADIO_UTF8)).SetCheck(charset == CharsetCode::UTF8);
    CButton(GetDlgItem(IDC_RADIO_UTF8BOM)).SetCheck(charset == CharsetCode::UTF8BOM);
    CButton(GetDlgItem(IDC_RADIO_GB18030)).SetCheck(charset == CharsetCode::GB18030);
    CButton(GetDlgItem(IDC_RADIO_OTHER)).SetCheck(Configuration::IsNormalCharset(charset) == false);

    GetDlgItem(IDC_COMBO_OTHER_CHARSET).EnableWindow(!isNormalCharset);
}

std::vector<std::tstring> DialogMain::AddItems(const std::vector<std::tstring> &pathes) {
    // 后缀
    unordered_set<tstring> filterDotExts;

    switch (core->GetConfig().filterMode) {
    case Configuration::FilterMode::NO_FILTER:
        break;
    case Configuration::FilterMode::SMART: // 智能识别文本
        break;
    case Configuration::FilterMode::ONLY_SOME_EXTANT:
        // 只包括指定后缀
        CheckAndTraversalIncludeRule([&](const tstring &dotExt) {
            filterDotExts.insert(dotExt);
        });
        break;
    default:
        assert(0);
    }

    vector<pair<tstring, tstring>> failed; // 失败的文件
    vector<tstring> ignored;               // 忽略的文件

    auto AddItemNoException = [&](const std::tstring &filename) {
        try {
            core->AddItem(filename, filterDotExts);
        } catch (io_error_ignore) { ignored.push_back(filename); } catch (runtime_error &e) {
            failed.push_back({filename, to_tstring(e.what())});
        }
    };

    for (auto &path : pathes) {
        // 如果是目录
        if (IsFolder(path)) {
            // 遍历指定目录
            auto filenames = TraversalAllFileNames(path);

            for (auto &filename : filenames) {
                if (doCancel) {
                    goto AddItemsAbort;
                }
                AddItemNoException(filename);
            }
            continue;
        }

        // 如果是文件
        if (doCancel) {
            goto AddItemsAbort;
        }
        AddItemNoException(path);
    }

AddItemsAbort:

    if (!failed.empty()) {
        tstring info = TEXT("以下文件添加失败：\r\n");
        for (auto &pr : failed) {
            info += pr.first + TEXT(" 原因：") + pr.second + TEXT("\r\n");
        }

        MyMessage *msg = new MyMessage([this, info]() {
            MessageBox(info.c_str(), TEXT("Error"), MB_OK | MB_ICONERROR);
        });
        cout << "AddItems MessageBox error " << std::hex << msg << endl;
        PostMessage(WM_MY_MESSAGE, 0, reinterpret_cast<LPARAM>(msg));
    }

    if (!ignored.empty()) {
        tstringstream ss;
        ss << to_tstring(ignored.size()) << TEXT(" 个文件被判定为非文本文件、或者没有探测出字符集：\r\n");

        int count = 0;
        for (auto &filename : ignored) {
            ss << filename << TEXT("\r\n");
            count++;

            if (count >= 5) {
                ss << TEXT("......等");
                break;
            }
        }

        wstring s = ss.str();
        PostUIFunc([this, s]() {
            MessageBox(s.c_str(), TEXT("提示"), MB_OK | MB_ICONINFORMATION);
        });
        return ignored;
    }

#ifndef NDEBUG
    cout << "Exit: AddItems" << endl;
#endif
    return ignored;
}

void DialogMain::StartConvert(const std::vector<std::pair<int, bool>> &restore, const std::vector<Item> &items) try {
    // 使用RTTI的手法记下恢复事件
    unique_ptr<void, function<void(void *)>> deferRestore(reinterpret_cast<void *>(1), [this, restore](void *) {
        PostUIFunc([this, restore]() {
            RestoreReadyState(restore);

#ifndef NDEBUG
            cout << "Exit: StartConvert thread" << endl;
#endif
            thRunning = false;
        });
    });

    // 如果没有内容
    if (listview.GetItemCount() == 0) {
        throw runtime_error("没有待转换的文件。");
    }

    // 检查输出目录
    if (core->GetConfig().outputTarget != Configuration::OutputTarget::ORIGIN) {
        if (core->GetConfig().outputDir.empty()) {
            throw runtime_error("输出目录无效。");
        }
    }

    vector<pair<tstring, tstring>> failed; // 失败文件/失败原因
    vector<tstring> succeed;               // 成功的文件

    // 目标编码
    auto targetCode = core->GetConfig().outputCharset;

    // 逐个转换
    auto count = items.size();
    for (int i = 0; i < count; ++i) {
        if (doCancel) {
            break;
        }

        auto &filename = items[i].filename;
        auto originCode = items[i].originCode;
        auto originLineBreak = items[i].originLineBreak;

        // 更新UI
        PostUIFunc([=]() {
            listview.SetItemText(i, static_cast<int>(ListViewColumn::INDEX), (TEXT("->") + to_tstring(i + 1)).c_str());
            listview.SetItemText(i, static_cast<int>(ListViewColumn::ENCODING), ToCharsetName(targetCode).c_str());
            // TODO
            // listview.SetItemText(i, static_cast<int>(ListViewColumn::LINE_BREAK), lineBreaksMap[lineBreak].c_str());

            // listview滚动
            listview.SelectItem(i);
        });

        auto [outputFileName, errInfo] = core->Convert(filename, originCode, targetCode, originLineBreak);
        if (errInfo.has_value()) {
            failed.push_back({filename, errInfo.value()});
        } else {
            succeed.push_back(filename);
        }

        // 更新UI
        PostUIFunc([=]() {
            listview.SetItemText(i, static_cast<int>(ListViewColumn::INDEX), to_tstring(i + 1).c_str());
            if (errInfo.has_value()) {
                return;
            }
            listview.SetItemText(i, static_cast<int>(ListViewColumn::FILENAME), outputFileName.c_str());
            listview.SetItemText(i, static_cast<int>(ListViewColumn::ENCODING), ToCharsetName(targetCode).c_str());
            // listview.SetItemText(i, static_cast<int>(ListViewColumn::LINE_BREAK), lineBreaksMap[lineBreak].c_str());
        });
    }

    // 已经完成处理

    // 如果有失败的
    if (failed.empty() == false) {
        tstringstream ss;
        ss << TEXT("转换成功 ") << succeed.size() << TEXT(" 个文件。\r\n\r\n");
        ss << TEXT("以下文件转换失败：\r\n");
        for (auto &pr : failed) {
            ss << pr.first << TEXT(" 原因：") << pr.second << TEXT("\r\n");
        }
        if (doCancel) {
            ss << TEXT("\r\n") << TEXT("\r\n") << TEXT("剩余文件由于取消操作所以未做处理。");
        }

        wstring s = ss.str();
        PostUIFunc([this, s]() {
            MessageBox(s.c_str(), TEXT("转换结果"), MB_OK | MB_ICONERROR);
        });
    } else {
        // 全部成功之后
        tstringstream ss;
        ss << TEXT("转换成功 ") << succeed.size() << TEXT(" 个文件。\r\n\r\n");

        if (targetCode == CharsetCode::GB18030) {
            ss << TEXT("\r\n\r\n注意：GB18030在纯英文的情况下和UTF-8编码位重合，所以可能会出现转换后显示为UTF-"
                       "8编码的情况。");
        }
        if (doCancel) {
            ss << TEXT("\r\n") << TEXT("\r\n") << TEXT("剩余文件由于取消操作所以未做处理。");
        }

        wstring s = ss.str();
        PostUIFunc([this, s]() {
            MessageBox(s.c_str(), TEXT("提示"), MB_OK | MB_ICONINFORMATION);
        });
    }

    return;
} catch (const runtime_error &err) {
    PostUIFunc([this, err]() {
        MessageBox(to_tstring(err.what()).c_str(), TEXT("出错"), MB_OK | MB_ICONERROR);
    });
    return;
}

LRESULT DialogMain::OnBnClickedRadioStretegyNoFilter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                                     BOOL & /*bHandled*/) {
    SetFilterMode(Configuration::FilterMode::NO_FILTER);
    return 0;
}

LRESULT DialogMain::OnBnClickedRadioStretegySmart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                                  BOOL & /*bHandled*/) {
    SetFilterMode(Configuration::FilterMode::SMART);
    return 0;
}

LRESULT DialogMain::OnBnClickedRadioStretegyManual(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                                   BOOL & /*bHandled*/) {
    SetFilterMode(Configuration::FilterMode::ONLY_SOME_EXTANT);
    return 0;
}

LRESULT DialogMain::OnBnClickedRadioToOrigin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                             BOOL & /*bHandled*/) {
    SetOutputTarget(Configuration::OutputTarget::ORIGIN);
    return 0;
}

LRESULT DialogMain::OnBnClickedRadioToDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    SetOutputTarget(Configuration::OutputTarget::TO_DIR);
    return 0;
}

LRESULT DialogMain::OnBnClickedRadioUtf8(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    SetOutputCharset(CharsetCode::UTF8);
    return 0;
}

LRESULT DialogMain::OnBnClickedRadioUtf8bom(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    SetOutputCharset(CharsetCode::UTF8BOM);
    return 0;
}

LRESULT DialogMain::OnBnClickedRadioGb18030(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    SetOutputCharset(CharsetCode::GB18030);
    return 0;
}

LRESULT DialogMain::OnBnClickedRadioOther(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    // SetOutputCharset(Configuration::OutputCharset::OTHER_UNSPECIFIED);
    return 0;
}

void DialogMain::CheckAndTraversalIncludeRule(std::function<void(const std::tstring &dotExt)> fn) {
    // 后缀字符串
    auto &extsStr = core->GetConfig().includeRule;

    // 切分
    auto exts = Split(extsStr, TEXT(" "));

    // 如果为空
    if (exts.empty()) {
        throw runtime_error("指定的后缀无效。\r\n\r\n例子：*.h *.hpp *.c *.cpp *.txt");
    }

    // 逐个检查
    for (auto ext : exts) {
        tstring extStr(ext);
        wstring pattern = TEXT(R"(\*(\.\w+))"); // 匹配 *.xxx 的正则
        wregex r(pattern);
        wsmatch results;
        if (regex_match(extStr, results, r) == false) {
            throw runtime_error("指定的后缀无效：" + to_string(extStr) +
                                "。\r\n\r\n例子： * .h * .hpp * .c * .cpp * .txt");
        }

        fn(results.str(1));
    }
}

LRESULT DialogMain::OnBnClickedButtonAddFiles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                              BOOL & /*bHandled*/) try {
    vector<pair<tstring, tstring>> dialogFilter;
    switch (core->GetConfig().filterMode) {
    case Configuration::FilterMode::NO_FILTER:
    case Configuration::FilterMode::SMART: // 智能识别文本
        dialogFilter = {{L"所有文件*.*", L"*.*"}};
        break;
    case Configuration::FilterMode::ONLY_SOME_EXTANT: {
        // 只包括指定后缀
        tstring filterExtsStr; // dialog的过滤器要求;分割
        CheckAndTraversalIncludeRule([&](const tstring &dotExt) {
            filterExtsStr += TEXT("*") + dotExt + TEXT(";");
        });

        // dialog过滤器
        dialogFilter.push_back(make_pair(filterExtsStr, filterExtsStr));

        break;
    }
    default:
        assert(0);
    }

    // 打开文件对话框
    TFileDialog dialog(*this, dialogFilter, true);
    if (dialog.Open()) {
        auto filenames = dialog.GetResult();

        AddItems(filenames);
    }
    return 0;
} catch (runtime_error &err) {
    MessageBox(to_tstring(err.what()).c_str(), TEXT("出错"), MB_OK | MB_ICONERROR);
    return 0;
}

LRESULT DialogMain::OnBnClickedButtonAddDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                            BOOL & /*bHandled*/) try {
    static tstring dir; // 可用于赋予TFolderBrowser初始路径

    TFolderBrowser folderBrowser(*this);
    if (folderBrowser.Open(dir)) {
        AddItems({dir});
    }

    return 0;
} catch (runtime_error &err) {
    MessageBox(to_tstring(err.what()).c_str(), TEXT("出错"), MB_OK | MB_ICONERROR);
    return 0;
}

LRESULT DialogMain::OnBnClickedButtonStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                           BOOL &bHandle /*bHandled*/) {
    if (thRunning) {
        cout << "OnBnClickedButtonStart set cancel true" << endl;
        doCancel = true;
        cout << "OnBnClickedButtonStart wait for fu.get()" << endl;
        fu.get();
        return 0;
    }

    auto restore = SetBusyState();

    vector<Item> items;
    for (int i = 0; i < listview.GetItemCount(); ++i) {
        auto filename = listview.GetItemText(i, static_cast<int>(ListViewColumn::FILENAME));
        auto originCode = ToCharsetCode(listview.GetItemText(i, static_cast<int>(ListViewColumn::ENCODING)));
        auto originLineBreak = lineBreaksMap[listview.GetItemText(i, static_cast<int>(ListViewColumn::LINE_BREAK))];
        items.push_back({filename, originCode, originLineBreak});
    }

    cout << "OnBnClickedButtonStart set cancel false" << endl;
    doCancel = false;
    cout << "OnBnClickedButtonStart async StartConvert" << endl;
    thRunning = true;
    fu = std::async(std::launch::async, &DialogMain::StartConvert, this, restore, items);
    return 0;
}

LRESULT DialogMain::OnBnClickedButtonClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    listview.DeleteAllItems();
    core->Clear();
    return 0;
}

LRESULT DialogMain::OnBnClickedButtonSetOutputDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                                  BOOL & /*bHandled*/) {
    tstring dir = core->GetConfig().outputDir;

    TFolderBrowser folderBrowser(*this);
    if (folderBrowser.Open(dir)) {
        core->SetOutputDir(dir);
        GetDlgItem(IDC_EDIT_OUTPUT_DIR).SetWindowTextW(dir.c_str());
    }

    return 0;
}

LRESULT DialogMain::OnCbnSelchangeComboOtherCharset(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                                    BOOL & /*bHandled*/) {
    // TODO: 在此添加控件通知处理程序代码

    return 0;
}

LRESULT DialogMain::OnNMRclickListview(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL & /*bHandled*/) {
    auto selectedItems = listview.GetSelectedItems();
    if (selectedItems.empty() == false) {
        PopupMenu(this->m_hWnd, IDR_MENU_RIGHT);
    }

    return 0;
}

LRESULT DialogMain::OnOpenWithNotepad(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    auto selectedItems = listview.GetSelectedItems();
    for (auto i : selectedItems) {
        auto filename = listview.GetItemText(i, static_cast<int>(ListViewColumn::FILENAME));

        wstring cmd = L"notepad " + filename;

        WinExec(to_string(cmd).c_str(), SW_SHOWNORMAL);
    }

    return 0;
}

LRESULT DialogMain::OnRemoveItem(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    auto selectedItems = listview.GetSelectedItems();
    for (auto itor = selectedItems.rbegin(); itor != selectedItems.rend(); ++itor) {
        int i = *itor;
        auto filename = listview.GetItemText(i, static_cast<int>(ListViewColumn::FILENAME));
        listview.DeleteItem(i);
        core->RemoveItem(filename);
    }
    return 0;
}

LRESULT DialogMain::OnEnChangeEditIncludeText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl,
                                              BOOL & /*bHandled*/) try {
    // 取得字符串
    tstring filterStr;

    BSTR bstr = nullptr;
    CEdit edit(hWndCtl);
    if (edit.GetWindowTextLengthW() != 0) {
        bool ok = edit.GetWindowTextW(bstr);
        if (!ok)
            throw runtime_error("出错：内存不足。");
        filterStr = bstr;
        SysReleaseString(bstr);
    }

    // 保存到core
    core->SetFilterRule(filterStr);

    return 0;
} catch (runtime_error &err) {
    MessageBox(to_tstring(err.what()).c_str(), TEXT("出错"), MB_OK | MB_ICONERROR);
    return 0;
}

LRESULT DialogMain::OnNMClickSyslink1(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL & /*bHandled*/) {
    HINSTANCE r = ShellExecute(NULL, L"open", L"https://github.com/tomwillow/SmartCharsetConverter/releases", NULL,
                               NULL, SW_SHOWNORMAL);

    return 0;
}

LRESULT DialogMain::OnBnClickedCheckConvertReturn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
                                                  BOOL & /*bHandled*/) {
    bool enableLineBreaks = CButton(GetDlgItem(IDC_CHECK_CONVERT_RETURN)).GetCheck();
    core->SetEnableConvertLineBreak(enableLineBreaks);

    CButton(GetDlgItem(IDC_RADIO_CRLF)).EnableWindow(enableLineBreaks);
    CButton(GetDlgItem(IDC_RADIO_LF)).EnableWindow(enableLineBreaks);
    CButton(GetDlgItem(IDC_RADIO_CR)).EnableWindow(enableLineBreaks);

    return 0;
}

LRESULT DialogMain::OnBnClickedRadioCrlf(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    core->SetLineBreaks(Configuration::LineBreaks::CRLF);

    return 0;
}

LRESULT DialogMain::OnBnClickedRadioLf(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    core->SetLineBreaks(Configuration::LineBreaks::LF);

    return 0;
}

LRESULT DialogMain::OnBnClickedRadioCr(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
    core->SetLineBreaks(Configuration::LineBreaks::CR);

    return 0;
}

LRESULT DialogMain::OnDropFiles(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) try {
    HDROP hDrop = reinterpret_cast<HDROP>(wParam);

    vector<tstring> filenames;
    UINT nFileNum = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); // 拖拽文件个数
    TCHAR strFileName[MAX_PATH];
    for (UINT i = 0; i < nFileNum; i++) {
        DragQueryFile(hDrop, i, strFileName, MAX_PATH); //获得拖曳的文件名
        filenames.push_back(strFileName);
    }
    DragFinish(hDrop); //释放hDrop

    auto restore = SetBusyState();

    // 添加文件
    // fu = thPool.submit([this, filenames, restore]() { AddItemsNoThrow(filenames, restore); });

    doCancel = false;
    thRunning = true;
    fu = std::async(std::launch::async, [this, restore, filenames]() {
        // 使用RTTI的手法记下恢复事件
        unique_ptr<void, function<void(void *)>> deferRestore(reinterpret_cast<void *>(1), [this, restore](void *) {
            PostUIFunc([this, restore]() {
                RestoreReadyState(restore);

#ifndef NDEBUG
                cout << "Exit: AddItemsNoThrow thread" << endl;
#endif
                thRunning = false;
            });
        });

        try {
            AddItems(filenames);
        } catch (const runtime_error &e) {
            PostUIFunc([this, e]() {
                MessageBox(to_tstring(e.what()).c_str(), TEXT("Error"), MB_OK | MB_ICONERROR);
            });
        }
    });

    return 0;
} catch (runtime_error &e) {
    MessageBox(to_tstring(e.what()).c_str(), TEXT("Error"), MB_OK | MB_ICONERROR);
    return 0;
}

void DialogMain::PostUIFunc(std::function<void()> fn) {
    MyMessage *msg = new MyMessage(fn);
    PostMessage(WM_MY_MESSAGE, 0, reinterpret_cast<LPARAM>(msg));
}

LRESULT DialogMain::OnUser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
    // cout << "OnUser Begin " << std::hex << lParam << endl;
    unique_ptr<MyMessage> msg(reinterpret_cast<MyMessage *>(lParam));
    msg->fn();
    // cout << "OnUser End " << std::hex << lParam << endl;
    return 0;
}

std::vector<pair<int, bool>> DialogMain::SetBusyState() noexcept {
    // 遍历控件，如果是启用状态，那么设置为disable，并在restore中记下，留待日后恢复
    vector<pair<int, bool>> restore;
    for (auto id = IDC_RADIO_STRETEGY_SMART; id <= IDC_RADIO_CR; ++id) {
        if (::GetDlgItem(m_hWnd, id) != NULL) {
            auto wnd = GetDlgItem(id);
            if (wnd.IsWindowEnabled()) {
                restore.push_back({id, true});
                wnd.EnableWindow(false);
            }
        }
    }

    // 开始按钮text变更为“取消”，并额外enable，用于让用户按“取消”
    GetDlgItem(IDC_BUTTON_START).SetWindowTextW(TEXT("取消"));
    GetDlgItem(IDC_BUTTON_START).EnableWindow(true);
    return restore;
}

void DialogMain::RestoreReadyState(const std::vector<std::pair<int, bool>> &restore) noexcept {
    for (auto &pr : restore) {
        auto wnd = GetDlgItem(pr.first);
        wnd.EnableWindow(pr.second);
    }

    GetDlgItem(IDC_BUTTON_START).SetWindowTextW(TEXT("开始转换"));
}
