// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// Created by Aeomanate on 07.04.2021.
//

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <winsock2.h>
#include <windows.h>
#include <conio.h>
#include "Client.hpp"
#include "../Sockets/Sockets.hpp"
#include "../Chat/Protocol.hpp"
using namespace std::chrono_literals;

[[maybe_unused]] static void test() {
    //auto udp_server = SocketAddress::ipv4FromDotNotation("192.168.0.104:60000");
    auto udp_server = SocketAddress::ipv4FromDotNotation("93.78.13.0:60000");
    
    auto send_socket = SocketUDP::create();
    send_socket->sendto("Hello, C++ Sockets!", *udp_server);
    std::cout << "\n";
    
    auto tcp_server = SocketAddress::ipv4FromDotNotation("93.78.13.0:60001");
    auto server = SocketTCP::create(*tcp_server);
    server->send("Hello, TCP sockets on C++!");
    
}

class CursorInLine {
  public:
    friend class Modifer;
    class Modifer {
      public:
        Modifer(CursorInLine* cursor_in_line, size_t cur_x)
            : cursor_in_line(cursor_in_line)
            , temp_x(cur_x)
        { }
        
        size_t& modify() {
            return temp_x;
        }
        
        ~Modifer() {
            cursor_in_line->setX(temp_x);
        }
      private:
        CursorInLine* cursor_in_line;
        size_t temp_x;
    };
    
  public:
    explicit CursorInLine(size_t offset)
    : h(GetStdHandle(STD_OUTPUT_HANDLE))
    , cur_x(0)
    , offset(offset)
    { }
    
    Modifer get() {
        return Modifer(this, cur_x);
    }
    
    size_t value() const {
        return cur_x;
    }
    
    void updateScreenPos() {
        setWindowCursor(cur_x, getWindowCursor().Y);
    }
    
  private:
    void setX(size_t x) {
        if(x != cur_x) setWindowCursor(cur_x = x, getWindowCursor().Y);
    }
    
    template <class T, class U>
    void setWindowCursor(T x, U y) {
        SetConsoleCursorPosition(h, { (short)x, (short)y });
    }
    
    COORD getWindowCursor() {
        CONSOLE_SCREEN_BUFFER_INFO buf;
        GetConsoleScreenBufferInfo(h, &buf);
        return buf.dwCursorPosition;
    }
    
  private:
    HANDLE h;
    size_t cur_x;
    size_t offset;
};

template <class T>
struct Is {
    T const& value;
    template <class... Args>
    bool in(Args... args) { return ((value == args) or ... or false); }
};
template <class T> Is<T> is(T const& d) { return Is<T>{d}; }

class CharEditor {
    enum SpecialCodes {
        ARROW_UP = 72,
        ARROW_DOWN = 80,
        ARROW_LEFT = 75,
        ARROW_RIGHT = 77,
        ENTER = 13,
        ESCAPE = 27,
        CLOSE = 107,
        BACKSPACE = 8,
        DEL = 83
    };
    
    struct Key {
        int m_code = 0;
        bool m_is_special = false;
    };
    
  public:
    explicit CharEditor(size_t username_offset, size_t m_max_chars)
    : m_cursor(username_offset)
    , m_max_chars(m_max_chars)
    { }
    
    std::string getUserMessage() {
        return m_chars;
    }
    
    void handleUserInput() {
        std::string temp_message;
        while(m_is_editor_work) {
            Key key = getKey();
            if(key.m_is_special) {
                controlHandler(key.m_code);
            } else {
                charHandler(key.m_code);
            }
    
            auto space = std::string(temp_message.size(), ' ');
            std::cout << "\r" << space << "\r";
            temp_message = getUserMessage();
            std::cout << temp_message;
            resetCursor();
        }
    }
    
    void resetCursor() {
        m_cursor.updateScreenPos();
    }
    
    bool isWork() const {
        return m_is_editor_work;
    }
    
  private:
    static Key getKey() {
        Key key = { _getch() };
        key.m_is_special = is(key.m_code).in(ENTER, ESCAPE, BACKSPACE, DEL);
        if(key.m_code == 224) {
            key.m_is_special = true;
            key.m_code = _getch();
        }
        return key;
    }
    
    void controlHandler(int code) {
        switch(code) {
            case ARROW_UP:
                m_cursor.get().modify() = 0;
                break;
            case ARROW_DOWN:
                m_cursor.get().modify() = m_chars.size();
                break;
            case ARROW_LEFT:
                if(m_cursor.value() > 0) m_cursor.get().modify() -= 1;
                break;
            case ARROW_RIGHT:
                if(m_cursor.value() < m_chars.size()) m_cursor.get().modify() += 1;
                break;
            case ENTER:
                break;
            case ESCAPE: [[fallthrough]];
            case CLOSE:
                if(m_chars.empty()) {
                    m_is_editor_work = false;
                } else {
                    m_chars.clear();
                    m_cursor.get().modify() = 0;
                }
                break;
            case BACKSPACE:
                if(m_cursor.value() == 0 or m_cursor.value() - 1 >= m_chars.size()) break;
                m_chars.erase(m_chars.begin() + (int)--m_cursor.get().modify());
                break;
            case DEL:
                if(m_cursor.value() >= m_chars.size()) break;
                m_chars.erase(m_chars.begin() + (int)m_cursor.value());
                break;
            default:
                std::cout << (int)code << "\n";
                break;
        }
    }
    
    void charHandler(int code) {
        if(m_max_chars and m_chars.size() <= m_max_chars)
        m_chars.insert(m_chars.begin() + (int)m_cursor.get().modify()++, (char)code);
    }
    
  private:
    std::string m_chars;
    CursorInLine m_cursor;
    bool m_is_editor_work = true;
    size_t m_max_chars;
};


void Client::run() {
    auto server = SocketAddress::ipv4FromDotNotation("77.123.102.243:60001");
    
    std::string username = "Aeomanate";
    auto chat_socket = ChatSocketTcp(SocketTCP::create(*server), username);
    chat_socket.sendMessage("Lol kek 祡�४");
    chat_socket.sendMessage("123456");
    chat_socket.sendMessage("OLOLOLOLOLOLOLOLOLOL");
}
