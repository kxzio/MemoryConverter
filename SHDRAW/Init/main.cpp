#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include "Include.h"
#include "CreateWndProc.h"
#include "WIndowProcess.h"
#include <vector>
#include <sstream>
#include "../fnv1a.h"
#include <time.h>
#include <filesystem>
#include <iostream>
#include <sys/types.h>
#include <iostream>
#include <Windows.h>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <experimental/filesystem>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <strsafe.h>
#include <exception>
#include <typeinfo>
#include <stdexcept>

#define BUFSIZE 512
float screen_max[]{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

ImVec2 operator-(const ImVec2& l, const ImVec2& r) { return{ l.x - r.x, l.y - r.y }; }
ImVec2 operator+(const ImVec2& l, const ImVec2& r) { return{ l.x + r.x, l.y + r.y }; }

int main()
{
    /* First argument if window pos - position, where will be rendered window and size, how big window will be */
    window.init( ImVec2{ 50, 50 }, ImVec2{ 480, 355 } );
    auto& IO = ImGui::get_io();

    ImFontConfig font_config;
    font_config.OversampleH = 1; //or 2 is the same
    font_config.OversampleV = 1;
    font_config.PixelSnapH = 1;

    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x044F, // Cyrillic
        0,
    };

    g_Menu.Default = IO.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/verdana.ttf", 13.f, &font_config);

    /* DIRECTX & wndproc initialization */
    m_directx.  init32(); 
    window.     process_msg();
    /* code shutdown  UPD: fixed crash in time of process closing & (device lost) */ 
    m_directx.  finish();

    return 0;
};

#define set_c(a, b) ImGui::SetCursorPos(ImVec2(a, b))

namespace fs = std::filesystem;

static std::vector<char> read_all_memory (char const* filename)
{
    std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    std::vector<char>  result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(&result[0], pos);

    return result;
}

static std::string err;

namespace global_menu {

    #define BUFSIZE 512

    TCHAR* get_data_from_handle (HANDLE hFile) 
    {
        BOOL bSuccess = FALSE;
        TCHAR pszFilename[MAX_PATH + 1];
        HANDLE hFileMap;

        // Get the file size.
        DWORD dwFileSizeHi = 0;
        DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

        if (dwFileSizeLo == 0 && dwFileSizeHi == 0)
        {
            _tprintf(TEXT("Cannot map a file with a length of zero.\n"));
            return FALSE;
        }

        // Create a file mapping object.
        hFileMap = CreateFileMapping(hFile,
            NULL,
            PAGE_READONLY,
            0,
            1,
            NULL);

        if (hFileMap)
        {
            // Create a file mapping to get the file name.
            void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

            if (pMem)
            {
                if (GetMappedFileName(GetCurrentProcess(),
                    pMem,
                    pszFilename,
                    MAX_PATH))
                {

                    //translate path with device name to drive letters.
                    TCHAR szTemp[BUFSIZE];
                    szTemp[0] = '\0';

                    if (GetLogicalDriveStrings(BUFSIZE - 1, szTemp))
                    {
                        TCHAR szName[MAX_PATH];
                        TCHAR szDrive[3] = TEXT(" :");
                        BOOL bFound = FALSE;
                        TCHAR* p = szTemp;

                        do
                        {
                            // Copy the drive letter to the template string
                            *szDrive = *p;

                            // Look up each device name
                            if (QueryDosDevice(szDrive, szName, MAX_PATH))
                            {
                                size_t uNameLen = _tcslen(szName);

                                if (uNameLen < MAX_PATH)
                                {
                                    bFound = _tcsnicmp(pszFilename, szName, uNameLen) == 0
                                        && *(pszFilename + uNameLen) == _T('\\');

                                    if (bFound)
                                    {
                                        // Reconstruct pszFilename using szTempFile
                                        // Replace device path with DOS path
                                        TCHAR szTempFile[MAX_PATH];
                                        StringCchPrintf(szTempFile,
                                            MAX_PATH,
                                            TEXT("%s%s"),
                                            szDrive,
                                            pszFilename + uNameLen);
                                        StringCchCopyN(pszFilename, MAX_PATH + 1, szTempFile, _tcslen(szTempFile));
                                    }
                                }
                            }

                            // Go to the next NULL character.
                            while (*p++);
                        } while (!bFound && *p); // end of string
                    }
                }
                bSuccess = TRUE;
                UnmapViewOfFile(pMem);
            }

        }
        return pszFilename;
    }

    std::wstring WSTRING_TO_LCPSWSTR (const std::string& s)
    {
        int len;
        int slength = (int)s.length() + 1;
        len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
        wchar_t* buf = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
        std::wstring r(buf);
        delete[] buf;
        return r;
    }

    void init() {
   

        //configurate menu :
        static const auto   background = ImGui::GetBackgroundDrawList();
        static const auto   foreground = ImGui::GetForegroundDrawList();
        const  auto         opened     = GetKeyState(VK_INSERT) & 1;

        //flags - 
        static const WORD flags
        = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

		ImGui::SetNextWindowSize(ImVec2(480, 355));

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        auto& style = ImGui::GetStyle();

        //fix locale
        setlocale(LC_ALL, "");

        //search file database
        WIN32_FIND_DATA find_dt;

        style.FrameRounding = 0.f;
        style.ScrollbarSize = 1.f;
        style.Colors[ImGuiCol_FrameBg] = ImColor(30 - 10, 30 - 10, 30 - 10);
        style.Colors[ImGuiCol_Border] = ImColor(30, 30, 30, 0);
        style.Colors[ImGuiCol_FrameBgHovered] = ImColor(35 - 10, 35 - 10, 35 - 10);
        style.Colors[ImGuiCol_FrameBgActive] = ImColor(40 - 10, 40 - 10, 40 - 10);
        style.Colors[ImGuiCol_Button] = ImColor(30, 30, 30);
        style.Colors[ImGuiCol_ButtonHovered] = ImColor(35, 35, 35);
        style.Colors[ImGuiCol_ButtonActive] = ImColor(40, 40, 40);
        style.Colors[ImGuiCol_TitleBg] = ImColor(30, 30, 30);
        style.Colors[ImGuiCol_TitleBgActive] = ImColor(30, 30, 30);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(30, 30, 30);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImColor(61, 135, 252);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(61, 135, 252);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(61, 135, 252);

		if (ImGui::Begin("ToByte | Build (Debug) : 05.04.2021", nullptr, flags))
		{

            const char* directory_name = "dtm";
            //after gui init, we should create right data directory
            static bool should_create_directory = true;

            //file ph
            FILE* fp;

            //buffer of status
            struct stat buff;

            //check, if we have to create same file
            should_create_directory = ((fp = fopen(directory_name, "rb")) == NULL);

            bool directory_was_created;
            
            if (should_create_directory)
            {
                //filesystem modulate
                directory_was_created = std::filesystem::create_directory((std::stringstream{ } << "C:\\" << directory_name).str());

                //assert, we can init parse system
                IM_ASSERT(!directory_was_created && "error#32 : filesystem cant init directory (try to startup app with administrator permissions)");
            }
            else
                directory_was_created = true;

            //path, that will parse files
            auto scan_path = L"C:\\dtm\\*";

            const char* scan_path_str = "C:\\dtm\\";

            //gui engine setup
            ImDrawList*  d = ImGui::GetWindowDrawList();

            const ImVec2 p = ImGui::GetWindowPos(), s = ImGui::GetWindowSize();

            ImGui::GetOverlayDrawList()->AddRectFilled(p + ImVec2(0, 19), p + ImVec2(s.x, 20), ImColor(61, 135, 252, 100));
            //data setup
            HANDLE __this_file_data;

            //get first file, then move handle to latest
            __this_file_data = FindFirstFile(scan_path, &find_dt);

            static bool selectable_data[256];

            std::vector<std::string>  VFile_path;

            static std::vector <std::string> Vfile_links;

            ImGui::ListBoxHeader("files list", ImVec2(200, 315));
            {
                int count = 0;

                if (__this_file_data != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        //dynamic filename
                        char* nPtr = new char[lstrlen(find_dt.cFileName) + 1];

                        //transform to new ch
                        for (int i = 0; i < lstrlen(find_dt.cFileName); i++)
                            nPtr[i] = char(find_dt.cFileName[i]);

                        //cl
                        nPtr[lstrlen(find_dt.cFileName)] = '\0';

                        if (nPtr != "." && nPtr != "..")
                        {
                            //check file validality
                            if (find_dt.nFileSizeLow > 0)
                            {
                                const std::string __str_thisname = (std::stringstream{ } << nPtr).str();
                                //check, if it is not c/c++ file type
                                if (__str_thisname.at(__str_thisname.length() - 1) != 'h')
                                {
                                    const bool clicked = ImGui::Selectable(nPtr, &selectable_data[count]);
                                    //set new ptr
                                    if (selectable_data[count])
                                    {
                                        //set new scan path
                                        VFile_path.push_back((std::stringstream{ } << scan_path_str << nPtr).str());
                                    }
                                    count++;
                                }
                            }
                        }


                    } while (FindNextFile(__this_file_data, &find_dt));
                }


            }    
            ImGui::ListBoxFooter();

            set_c(215, 25);

            static bool force_delete = true;

            if (ImGui::BeginChild("database_child##1"))
            {
                //validality check
                if (!VFile_path.empty())
                {
                    for (auto p : VFile_path)
                        ImGui::Text(p.c_str());

                    ImGui::Text("Files configuration : ");

                    static bool transform;
                    static int itimer = 0;

                    if (ImGui::Button("Convert", ImVec2(255, 25)))  
                    {
                        transform = true;
                    }

                    if (transform)
                    {
                        //timer system, cause when we start byte converting, render system is cannot render in this time
                        itimer++;           
                        //background overlay
                        ImGui::GetOverlayDrawList()->AddRectFilled(p, p + s, ImColor(0, 0, 0, 150));
                        //text overlay "Please wait"
                        ImGui::GetOverlayDrawList()->AddText(p + ImVec2(s.x / 2 - ImGui::CalcTextSize("Please wait...").x / 2, s.y / 2), ImColor(255, 255, 255), "Please wait...");
                        //start system after render normalization
                        if (itimer > 1)
                        {
                            for (size_t i = 0; i < VFile_path.size(); i++)
                            {
                                //record out path data
                                const fs::path __file_record_path
                                    = VFile_path[i];

                                //open ios record with opentype : binary
                                std::ifstream __binary_ios_record
                                (__file_record_path, std::ios::binary);

                                //assert warning : debug
                                //IM_ASSERT(__binary_ios_record.fail() && "#cannot open source file");

                                //prop seekg : end (Off, Way)
                                __binary_ios_record.seekg(0, std::ios_base::end);

                                //output size file
                                const size_t sizeFile = static_cast <size_t> (__binary_ios_record.tellg());

                                //prop seekg : begin (Off, Way)
                                __binary_ios_record.seekg(0, __binary_ios_record.beg);

                                std::string _out_file_name = __file_record_path.stem().string();

                                //add (.h) - c/c++ read/write files
                                std::string output_file = _out_file_name.append(".h");

                                //add path
                                std::string new_path_name = scan_path_str + output_file;
                                std::ofstream stl_offstream__converted
                                (new_path_name, std::ios::trunc); // open type : ok

                                //feature
                                stl_offstream__converted << "//converted to byte array by prism 2021 : (brokencore) " << std::endl;
                                //delete (.h)
                                output_file.erase(output_file.length() - 2, 2);
                                //add var 
                                stl_offstream__converted << "constexpr int " << output_file << "_size" << " = " << sizeFile << ";" << std::endl;;
                                //create array (C)
                                stl_offstream__converted << "const unsigned char " << output_file << "[" << sizeFile << "]" << std::endl;
                                //open
                                stl_offstream__converted << "{" << std::endl;

                                stl_offstream__converted << "    ";

                                std::stringstream _sstream;

                                // set basefield to hex
                                _sstream << std::hex;

                                //FUNCTION TEMPLATE setfill
                                _sstream << std::uppercase << std::setfill('0');

                                std::string separator = "";

                                int icol = 0;

                                // CLASS TEMPLATE istreambuf_iterator
                                auto it = std::istreambuf_iterator<char>(__binary_ios_record);
                                auto eof = std::istreambuf_iterator<char>();

                                do
                                {
                                    //c
                                    unsigned int c = *it++ & 0xFF;
                                    //INSTANTIATIONS
                                    _sstream << separator << "0x" << std::setw(2) << c;
                                    //change separator
                                    separator = ", ";
                                    //mem
                                    if (++icol % 16 == 0)
                                    {
                                        if (it != eof)
                                        {
                                            _sstream << "," << std::endl;
                                            _sstream << "\t";
                                            separator = "";
                                        }
                                    }

                                } while (!it.equal(eof));

                                stl_offstream__converted << _sstream.str();
                                // insert newline and flush stream
                                stl_offstream__converted << std::endl;
                                //close
                                stl_offstream__converted << "};" << std::endl;

                                //close file edit
                                __binary_ios_record.close();
                                //_HAS_OLD_IOSTREAMS_MEMBERS
                                stl_offstream__converted.close();

                                if (force_delete)
                                {
                                    //delete old original source
                                    std::filesystem::remove(VFile_path[i]);
                                }
                                //clear file path
                            }
                            VFile_path.clear();
                            //clear selectables info
                            for (int i = 0; i < 256; i++)
                            {
                                selectable_data[i] = false;
                            }
                            transform = false;
                            itimer = 0;
                        }
                    }

                }

                //add dialog box database
                OPENFILENAME ofn;

                //ouput file hanler
                static HANDLE file_handle;              

                //output file 
                TCHAR file_name [260] = { NULL };

                if (ImGui::Button("Add file", ImVec2(255, 25)))
                {

                    // Initialize OPENFILENAME
                    ZeroMemory(&ofn, sizeof(ofn));

                    ofn.lStructSize = sizeof(ofn);
                    //hwnd owner setup (from windowprocess)

                    ofn.hwndOwner = wnd.hwnd;

                    ofn.lpstrFile = file_name;
                    //maximal size file

                    ofn.nMaxFile = sizeof(file_name);

                    ofn.lpstrFilter = _T("All\0*.*\0Text\0*.TXT\0");
                    //filter = none
                    ofn.nFilterIndex = 1;
                    ofn.lpstrFileTitle = NULL;

                    //title 0 
                    ofn.nMaxFileTitle = 0;
                    ofn.lpstrInitialDir = NULL;

                    //validality deafault flags
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                }


                if (GetOpenFileName(&ofn) == TRUE)
                {
                    file_handle = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, ( LPSECURITY_ATTRIBUTES ) NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, ( HANDLE ) NULL);
                }

                std::wstring converted_wstr;

                if (file_handle)
                {
                    auto __tchar_path = get_data_from_handle(file_handle);

                    converted_wstr = (&__tchar_path[0]);
                    //convert to wstring
                     
                    std::string __path_str (converted_wstr.begin(), converted_wstr.end()); 
                    //and convert to string.

                    std::filesystem::path orig_path { __path_str };
                    //copy original file and move it to "C:\\dtm\\"

                    const std::filesystem::path src = __path_str.c_str();

                    ShellExecute(nullptr, TEXT("open"), TEXT("C:\\dtm\\"), nullptr, nullptr, 1);

                    file_handle = nullptr;
                }

                if (ImGui::Button("Open folder", ImVec2(255, 25)))
                {
                    ShellExecute(nullptr, TEXT("open"), TEXT("C:\\dtm\\"), nullptr, nullptr, 1);
                }

                if (ImGui::Button("Telegram blog", ImVec2(255, 25)))
                {
                    ShellExecute(nullptr, TEXT("open"), TEXT("https://t.me/prism_blog"), nullptr, nullptr, 1);
                }

                ImGui::Checkbox("Delete after convert", &force_delete);

                ImGui::Text("Developed by prism");


            }
            ImGui::EndChild();

            //ImGui::Text(err.c_str());



        }
        ImGui::End();
		

    }


};
