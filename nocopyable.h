#pragma once
//被继承以后派生类对象不能拷贝和拷贝赋值
class nocopyable {
public:
    nocopyable(const nocopyable&)=delete;
    nocopyable& operator=(const nocopyable&)=delete;
protected:
    nocopyable()=default;
    ~nocopyable()=default;
};