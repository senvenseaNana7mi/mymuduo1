#include "Timestamp.h"
#include <bits/types/time_t.h>
#include <cstddef>
#include <cstdio>
#include<time.h>

Timestamp::Timestamp():microSecondsSinceEpoch_(0){}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch){}
Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
}

std::string Timestamp::toString() const
{
    char buf[128]={0};
    tm *tm_t = localtime(&microSecondsSinceEpoch_);//long int
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
        tm_t->tm_year+1990,
        tm_t->tm_mon+1,
        tm_t->tm_mday,
        tm_t->tm_hour,
        tm_t->tm_min,
        tm_t->tm_sec);
    return buf;
}