// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ranges_extras.h"
#include "memberproxy.h"

namespace Quotient {

class Room;

//! \brief A list of room member states
class QUOTIENT_API MemberListProxy : public std::ranges::view_interface<MemberListProxy>
{
    Q_GADGET
    QML_NAMED_ELEMENT(memberListProxy)
    QML_UNCREATABLE("")
    QML_SEQUENTIAL_CONTAINER(Quotient::MemberProxy)

    Q_PROPERTY(bool isEmpty READ isEmpty CONSTANT)
    Q_PROPERTY(QStringList ids READ ids CONSTANT)
public:
    MemberListProxy(const Room *room, QList<const RoomMemberEvent *> memberEvents);
    ~MemberListProxy();

    template <typename RangeT>
        requires std::convertible_to<std::ranges::range_reference_t<RangeT>, const RoomMemberEvent *>
    MemberListProxy(std::from_range_t, RangeT &&range, const Room *room)
        : MemberListProxy(room, rangeTo<QList>(std::forward<RangeT>(range)))
    {}

    auto count() const { return _memberEvents.count(); }
    auto size() const { return _memberEvents.size(); }
    bool isEmpty() const { return _memberEvents.isEmpty(); }

    QStringList ids() const { return _memberIds; }

    void sort(std::invocable<const RoomMemberEvent*, const RoomMemberEvent*> auto sorter)
    {
        std::ranges::sort(_memberEvents, sorter);
        fillMemberIds();
    }

    MemberProxy firstNonLocal() const;

    class iterator
    {
        using base_iter_t = QList<const RoomMemberEvent *>::const_iterator;
    public:
        struct Proxy
        {
            MemberProxy value;
            const MemberProxy *operator->() const { return &value; }
        };

        using difference_type = base_iter_t::difference_type;
        using value_type = MemberProxy;
        using reference = const value_type; // We only allow const access for now
        using pointer = Proxy;
        using iterator_concept = base_iter_t::iterator_concept;

        reference operator*() const { return MemberProxy(_room, *_it); }
        pointer operator->() const { return pointer{MemberProxy(_room, *_it)}; }
        reference operator[](difference_type n) const { return *iterator(_room, _it + n); }

        iterator &operator++();
        iterator operator++(int);
        iterator &operator--();
        iterator operator--(int);

        iterator &operator+=(difference_type n);
        iterator &operator-=(difference_type n);
        friend iterator operator+(iterator it, difference_type n) { return it += n; }
        friend iterator operator-(iterator it, difference_type n) { return it + (-n); }
        friend iterator operator+(difference_type n, iterator it) { return it + n; }
        friend difference_type operator-(iterator lhs, iterator rhs) { return lhs._it - rhs._it; }

        friend auto operator==(iterator lhs, iterator rhs) { return lhs._it == rhs._it; }
        friend std::partial_ordering operator<=>(iterator lhs, iterator rhs)
        {
            return lhs.compareWith(std::move(rhs));
        }

    private:
        const Room *_room;
        base_iter_t _it;
        iterator(const Room *room, base_iter_t it) : _room(room), _it(it) {}
        friend class MemberListProxy;

        std::partial_ordering compareWith(const iterator rhs) const;
    };

    iterator begin() const { return {_room, _memberEvents.begin()}; }
    iterator end() const { return {_room, _memberEvents.end()}; }
    iterator cbegin() const { return begin(); }
    iterator cend() const { return end(); }

private:
    const Room *_room = nullptr;
    QMetaObject::Connection _roomListener{};

    QList<const RoomMemberEvent *> _memberEvents{};
    QStringList _memberIds{};

    void fillMemberIds();
};

} // namespace Quotient
