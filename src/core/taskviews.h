#ifndef STARS_TASKVIEWS_H
#define STARS_TASKVIEWS_H

#include <stdint.h>
#include <type_traits>

// These call-scoped aliases refer to real task words. They carry no storage in
// ORDER and copy values on assignment, never reference bindings.
template<class T>
struct NATIVETASKLAYMINESVIEW {
    static_assert(std::is_same_v<std::remove_const_t<T>, uint16_t>);
    T &cTime;
    T &cTimeOld;
    NATIVETASKLAYMINESVIEW(T &cTime, T &cTimeOld) noexcept : cTime(cTime), cTimeOld(cTimeOld) {}
    NATIVETASKLAYMINESVIEW(const NATIVETASKLAYMINESVIEW &) = default;
    NATIVETASKLAYMINESVIEW &operator=(const NATIVETASKLAYMINESVIEW &other) noexcept requires (!std::is_const_v<T>) {
        return Assign(other);
    }
    template<class U>
    NATIVETASKLAYMINESVIEW &operator=(const NATIVETASKLAYMINESVIEW<U> &other) noexcept requires (!std::is_const_v<T>) {
        return Assign(other);
    }
private:
    template<class U>
    NATIVETASKLAYMINESVIEW &Assign(const NATIVETASKLAYMINESVIEW<U> &other) noexcept {
        uint16_t cTimeNew = other.cTime;
        uint16_t cTimeOldNew = other.cTimeOld;
        cTime = cTimeNew;
        cTimeOld = cTimeOldNew;
        return *this;
    }
};

template<class T>
struct NATIVETASKPATROLVIEW {
    static_assert(std::is_same_v<std::remove_const_t<T>, uint16_t>);
    T &iWarp;
    T &iDist;
    NATIVETASKPATROLVIEW(T &iWarp, T &iDist) noexcept : iWarp(iWarp), iDist(iDist) {}
    NATIVETASKPATROLVIEW(const NATIVETASKPATROLVIEW &) = default;
    NATIVETASKPATROLVIEW &operator=(const NATIVETASKPATROLVIEW &other) noexcept requires (!std::is_const_v<T>) {
        return Assign(other);
    }
    template<class U>
    NATIVETASKPATROLVIEW &operator=(const NATIVETASKPATROLVIEW<U> &other) noexcept requires (!std::is_const_v<T>) {
        return Assign(other);
    }
private:
    template<class U>
    NATIVETASKPATROLVIEW &Assign(const NATIVETASKPATROLVIEW<U> &other) noexcept {
        uint16_t iWarpNew = other.iWarp;
        uint16_t iDistNew = other.iDist;
        iWarp = iWarpNew;
        iDist = iDistNew;
        return *this;
    }
};

template<class T>
struct NATIVETASKSELLVIEW {
    static_assert(std::is_same_v<std::remove_const_t<T>, uint16_t>);
    T &iPlrX;
    NATIVETASKSELLVIEW(T &iPlrX) noexcept : iPlrX(iPlrX) {}
    NATIVETASKSELLVIEW(const NATIVETASKSELLVIEW &) = default;
    NATIVETASKSELLVIEW &operator=(const NATIVETASKSELLVIEW &other) noexcept requires (!std::is_const_v<T>) {
        return Assign(other);
    }
    template<class U>
    NATIVETASKSELLVIEW &operator=(const NATIVETASKSELLVIEW<U> &other) noexcept requires (!std::is_const_v<T>) {
        return Assign(other);
    }
private:
    template<class U>
    NATIVETASKSELLVIEW &Assign(const NATIVETASKSELLVIEW<U> &other) noexcept {
        uint16_t iPlrXNew = other.iPlrX;
        iPlrX = iPlrXNew;
        return *this;
    }
};

#endif
