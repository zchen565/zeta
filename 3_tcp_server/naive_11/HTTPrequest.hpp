#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>

#include "buffer.hpp"

// static 初始化问题
// OOR 问题
const std::unordered_set<std::string> HTTPrequest::DEFAULT_HTML {
            "/index", "/welcome", "/video", "/picture"};

/**
 * @brief HTTP request parse
 * This is only a key value parse, not the complete state machine implementation
 * 
 */
class HTTPrequest {
public:
    enum PARSE_STATE{
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

private:

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string,std::string> header_;
    std::unordered_map<std::string,std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;

// func
private:
    //解析请求行
    bool parseRequestLine(const std::string& line) {
        std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
        std::smatch subMatch;
        if(regex_match(line, subMatch, patten)) {   
            method_ = subMatch[1];
            path_ = subMatch[2];
            version_ = subMatch[3];
            state_ = HEADERS;
            return true;
        }
        return false;
    }

    //解析请求头
    void parseRequestHeader(const std::string& line) {
        std::regex patten("^([^:]*): ?(.*)$");
        std::smatch subMatch;
        if(regex_match(line, subMatch, patten)) {
            header_[subMatch[1]] = subMatch[2];
        } else {
            state_ = BODY;
        }
    }

    //解析数据体
    void parseDataBody(const std::string& line) {
        body_ = line;
        parsePost();
        state_ = FINISH;
    }

    void parsePath() {
        if(path_ == "/") {
            path_ = "/index.html"; 
        }
        else {
            for(auto &item: DEFAULT_HTML) {
                if(item == path_) {
                    path_ += ".html";
                    break;
                }
            }
        }
    }

    void parsePost() {
        if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
            if(body_.size() == 0) { return; }

            std::string key, value;
            int num = 0;
            int n = body_.size();
            int i = 0, j = 0;

            for(; i < n; i++) {
                char ch = body_[i];
                switch (ch) {
                case '=':
                    key = body_.substr(j, i - j);
                    j = i + 1;
                    break;
                case '+':
                    body_[i] = ' ';
                    break;
                case '%':
                    num = convertHex(body_[i + 1]) * 16 + convertHex(body_[i + 2]);
                    body_[i + 2] = num % 10 + '0';
                    body_[i + 1] = num / 10 + '0';
                    i += 2;
                    break;
                case '&':
                    value = body_.substr(j, i - j);
                    j = i + 1;
                    post_[key] = value;
                    break;
                default:
                    break;
                }
            }
            assert(j <= i);
            if(post_.count(key) == 0 && j < i) {
                value = body_.substr(j, i - j);
                post_[key] = value;
            }
        }   
    }

    static int convertHex(char ch) {
        if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
        if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
        return ch;
    }

public:
    HTTPrequest() { Init();};
    ~HTTPrequest() = default;

public:
    void Init() {
        method_ = path_ = version_ = body_ = "";
        state_ = REQUEST_LINE;
        header_.clear();
        post_.clear();
    }

    //解析HTTP请求
    bool Parse(Buffer& buff) {
        const char CRLF[] = "\r\n";
        if(buff.ReadableBytes() <= 0) {
            return false;
        }
        //std::cout<<"parse buff start:"<<std::endl;
        //buff.printContent();
        //std::cout<<"parse buff finish:"<<std::endl;
        while(buff.ReadableBytes() && state_ != FINISH) {
            const char* lineEnd = std::search(buff.CurReadPtr(), buff.CurWritePtrConst(), CRLF, CRLF + 2);
            std::string line(buff.CurReadPtr(), lineEnd);
            switch(state_)
            {
            case REQUEST_LINE:
                //std::cout<<"REQUEST: "<<line<<std::endl;
                if(!parseRequestLine(line)) {
                    return false;
                }
                parsePath();
                break;    
            case HEADERS:
                parseRequestHeader(line);
                if(buff.ReadableBytes() <= 2) {
                    state_ = FINISH;
                }
                break;
            case BODY:
                parseDataBody(line);
                break;
            default:
                break;
            }
            if(lineEnd == buff.CurWritePtr()) { break; }
            buff.UpdateReadPtrToEnd(lineEnd + 2);
        }
        return true;
    }

    //获取HTTP信息
    std::string Path() const {
        return path_;
    }
    std::string& Path() {
        return path_;
    }
    std::string Method() const {
        return method_;
    }
    std::string Version() const {
        return version_;
    }
    std::string GetPost(const std::string& key) const {
        assert(key != "");
        if(post_.count(key) == 1) {
            return post_.find(key)->second;
        }
        return "";
    }
    std::string GetPost(const char* key) const {
        assert(key != nullptr);
        if(post_.count(key) == 1) {
            return post_.find(key)->second;
        }
        return "";
    }

    bool IsKeepAlive() const {
        if(header_.count("Connection") == 1) {
            return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
        }
        return false;
    }
};
