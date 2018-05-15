//-------------------------------------------------------------------
//  Nano RPC
//  https://github.com/tdv/nanorpc
//  Created:     05.2018
//  Copyright (C) 2018 tdv
//-------------------------------------------------------------------

#ifndef __NANO_RPC_PACKER_PLAIN_TEXT_H__
#define __NANO_RPC_PACKER_PLAIN_TEXT_H__

// STD
#include <cassert>
#include <cstdint>
#include <istream>
#include <iterator>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

// BOOST
#include <boost/core/ignore_unused.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>

// NANORPC
#include "nanorpc/core/type.h"
#include "nanorpc/packer/detail/to_tuple.h"
#include "nanorpc/packer/detail/traits.h"

namespace nanorpc::packer
{

class plain_text final
{
private:
    class serializer;
    class deserializer;

public:
    using serializer_type = serializer;
    using deserializer_type = deserializer;

    template <typename T>
    serializer pack(T const &value)
    {
        return serializer{}.pack(value);
    }

    deserializer from_buffer(core::type::buffer buffer)
    {
        return deserializer{std::move(buffer)};
    }

private:
    class serializer final
    {
    public:
        serializer(serializer &&) noexcept = default;
        serializer& operator = (serializer &&) noexcept = default;
        ~serializer() noexcept = default;

        template <typename T>
        serializer pack(T const &value)
        {
            assert(stream_ && "Empty stream.");
            if (!stream_)
                throw std::runtime_error{"[" + std::string{__func__ } + "] Empty stream."};

            pack_value(value);
            return std::move(*this);
        }

        core::type::buffer to_buffer()
        {
            assert(stream_ && "Empty stream.");
            if (!stream_)
                throw std::runtime_error{"[" + std::string{__func__ } + "] Empty stream."};

            stream_.reset();
            auto tmp = std::move(*buffer_);
            buffer_.reset();
            return tmp;
        }

    private:
        using buffer_ptr = std::unique_ptr<core::type::buffer>;
        using stream_type = boost::iostreams::filtering_ostream;
        using stream_type_ptr = std::unique_ptr<stream_type>;

        buffer_ptr buffer_{std::make_unique<core::type::buffer>()};
        stream_type_ptr stream_{std::make_unique<stream_type>(boost::iostreams::back_inserter(*buffer_))};

        friend class plain_text;
        serializer() = default;

        serializer(serializer const &) = delete;
        serializer& operator = (serializer const &) = delete;

        template <typename T>
        static constexpr auto is_serializable(T &value) noexcept ->
                decltype(*static_cast<std::ostream *>(nullptr) << value, std::declval<std::true_type>());
        static constexpr std::false_type is_serializable(...) noexcept;
        template <typename T>
        static constexpr bool is_serializable_v = std::decay_t<decltype(is_serializable(*static_cast<T *>(nullptr)))>::value;

        template <typename T>
        std::enable_if_t<is_serializable_v<T> , void>
        pack_value(T const &value)
        {
            *stream_ << value << ' ';
        }

        template <typename T>
        std::enable_if_t<!is_serializable_v<T> && detail::traits::is_tuple_v<T>, void>
        pack_value(T const &value)
        {
            pack_tuple(value, std::make_index_sequence<std::tuple_size_v<T>>{});
        }

        template <typename T>
        std::enable_if_t<!is_serializable_v<T> && detail::traits::is_iterable_v<T>, void>
        pack_value(T const &value)
        {
            pack_value(value.size());
            for (auto const &i : value)
                pack_value(i);
        }

        template <typename T>
        std::enable_if_t
            <
                std::tuple_size_v<std::decay_t<decltype(detail::to_tuple(std::declval<std::decay_t<T>>()))>> != 0,
                void
            >
        pack_user_defined_type(T const &value)
        {
            pack_value(detail::to_tuple(value));
        }

        template <typename T>
        std::enable_if_t
            <
                !is_serializable_v<T> && !detail::traits::is_iterable_v<T> &&
                    !detail::is_tuple_v<T> && std::is_class_v<T>,
                void
            >
        pack_value(T const &value)
        {
            pack_user_defined_type(value);
        }

        template <typename ... T, std::size_t ... I>
        void pack_tuple(std::tuple<T ... > const &tuple, std::index_sequence<I ... >)
        {
            auto pack_tuple_item = [this] (auto value)
                {
                    pack_value(value);
                    *stream_ << ' ';
                };
            boost::ignore_unused(pack_tuple_item);
            (pack_tuple_item(std::get<I>(tuple)) , ... );
        }
    };

    class deserializer final
    {
    public:
        deserializer(deserializer &&) noexcept = default;
        deserializer& operator = (deserializer &&) noexcept = default;
        ~deserializer() noexcept = default;

        template <typename T>
        deserializer unpack(T &value)
        {
            assert(stream_ && "Empty stream.");
            if (!stream_)
                throw std::runtime_error{"[" + std::string{__func__ } + "] Empty stream."};

            unpack_value(value);
            return std::move(*this);
        }

    private:
        using buffer_ptr = std::unique_ptr<core::type::buffer>;
        using source_type = boost::iostreams::basic_array_source<char>;
        using source_type_ptr = std::unique_ptr<source_type>;
        using stream_type = boost::iostreams::stream<source_type>;
        using stream_type_ptr = std::unique_ptr<stream_type>;

        buffer_ptr buffer_;
        source_type_ptr source_{std::make_unique<source_type>(!buffer_->empty() ? buffer_->data() : nullptr, buffer_->size())};
        stream_type_ptr stream_{std::make_unique<stream_type>(*source_)};

        friend class plain_text;

        deserializer(deserializer const &) = delete;
        deserializer& operator = (deserializer const &) = delete;

        deserializer(core::type::buffer buffer)
            : buffer_{std::make_unique<core::type::buffer>(std::move(buffer))}
        {
        }

        template <typename T>
        static constexpr auto is_deserializable(T &value) noexcept ->
                decltype(*static_cast<std::istream *>(nullptr) >> value, std::declval<std::true_type>());
        static constexpr std::false_type is_deserializable(...) noexcept;
        template <typename T>
        static constexpr bool is_deserializable_v = std::decay_t<decltype(is_deserializable(*static_cast<std::decay_t<T> *>(nullptr)))>::value;

        template <typename T>
        std::enable_if_t<is_deserializable_v<T> , void>
        unpack_value(T &value)
        {
            (void)value;
            *stream_ >> value;
        }

        template <typename T>
        std::enable_if_t<!is_deserializable_v<T> && detail::traits::is_tuple_v<T>, void>
        unpack_value(T &value)
        {
            unpack_tuple(value, std::make_index_sequence<std::tuple_size_v<T>>{});
        }

        template <typename K, typename V>
        void unpack_value(std::pair<K const, V> &value)
        {
            using key_type = std::decay_t<K>;
            std::pair<key_type, V> pair;
            unpack_value(pair.first);
            unpack_value(pair.second);
            std::exchange(const_cast<key_type &>(value.first), pair.first);
            std::exchange(value.second, pair.second);
        }

        template <typename T>
        std::enable_if_t<!is_deserializable_v<T> && detail::traits::is_iterable_v<T>, void>
        unpack_value(T &value)
        {
            using size_type = typename std::decay_t<T>::size_type;
            using value_type = typename std::decay_t<T>::value_type;
            size_type count{};
            unpack_value(count);
            for (size_type i = 0 ; i < count ; ++i)
            {
                value_type item;
                unpack_value(item);
                *std::inserter(value, end(value)) = std::move(item);
            }
        }

        template <typename T, typename Tuple, std::size_t ... I>
        T make_from_tuple(Tuple && tuple, std::index_sequence<I ... >)
        {
            return T{std::move(std::get<I>(std::forward<Tuple>(tuple))) ... };
        }

        template <typename T>
        std::enable_if_t
            <
                std::tuple_size_v<std::decay_t<decltype(detail::to_tuple(std::declval<std::decay_t<T>>()))>> != 0,
                void
            >
        unpack_user_defined_type(T &value)
        {
            using tuple_type = std::decay_t<decltype(detail::to_tuple(value))>;
            tuple_type tuple;
            unpack_value(tuple);
            value = make_from_tuple<std::decay_t<T>>(std::move(tuple),
                    std::make_index_sequence<std::tuple_size_v<tuple_type>>{});
        }

        template <typename T>
        std::enable_if_t
            <
                !is_deserializable_v<T> && !detail::traits::is_iterable_v<T> &&
                    !detail::is_tuple_v<T> && std::is_class_v<T>,
                void
            >
        unpack_value(T &value)
        {
            unpack_user_defined_type(value);
        }

        template <typename ... T, std::size_t ... I>
        void unpack_tuple(std::tuple<T ... > &tuple, std::index_sequence<I ... >)
        {
            (unpack_value(std::get<I>(tuple)) , ... );
        }
    };

};

}   // namespace nanorpc::packer

#endif  // !__NANO_RPC_PACKER_PLAIN_TEXT_H__