#pragma once

#include <unordered_map>

#include <fcntl.h>  //open
#include <unistd.h> //close
#include <sys/stat.h> //stat
#include <sys/mman.h> //mmap,munmap
#include <assert.h>

#include "buffer.hpp"

class HTTPresponse {
private:
    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char* mmFile_;
    struct  stat mmFileStat_;

    // static init in the end of file
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;


private:
    void addStateLine(Buffer& buffer);
    void addResponseHeader(Buffer& buffer);
    void addResponseContent(Buffer& buffer);

    void errorHTML();
    std::string getFileType();

public:
    HTTPresponse() {
        code_ = -1;
        path_ = srcDir_ = "";
        isKeepAlive_ = false;
        mmFile_ = nullptr; 
        mmFileStat_ = { 0 };
    }
    ~HTTPresponse() {
        UnmapFile();
    }

public:
    void Init(const std::string& srcDir,std::string& path,bool isKeepAlive=false,int code=-1) {
        assert(srcDir != "");
        if(mmFile_) { UnmapFile(); }
        code_ = code;
        isKeepAlive_ = isKeepAlive;
        path_ = path;
        srcDir_ = srcDir;
        mmFile_ = nullptr; 
        mmFileStat_ = { 0 };        
    }
    void MakeResponse(Buffer& buffer) {
        /* 判断请求的资源文件 */
        if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
            code_ = 404;
        }
        else if(!(mmFileStat_.st_mode & S_IROTH)) {
            code_ = 403;
        }
        else if(code_ == -1) { 
            code_ = 200; 
        }
        errorHTML();
        addStateLine(buffer);
        addResponseHeader(buffer);
        addResponseContent(buffer);
    }

    void UnmapFile() {

    }

    char* File() {
        return mmFile_;
    }
    size_t FileLen() const {
        return mmFileStat_.st_size;
    }
    
    void ErrorContent(Buffer& buffer,std::string message);
    int Code() const {return code_;}

};


const std::unordered_map<std::string, std::string> HTTPresponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HTTPresponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HTTPresponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};