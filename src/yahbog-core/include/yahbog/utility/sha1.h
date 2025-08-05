#pragma once

#include <cstdint>
#include <array>
#include <span>
#include <string_view>
#include <algorithm>
#include <bit>

namespace yahbog {
	// A reimplementation of the TinySHA1 library. The original license follows:

	/*
	 *
	 * TinySHA1 - a header only implementation of the SHA1 algorithm in C++. Based
	 * on the implementation in boost::uuid::details.
	 *
	 * SHA1 Wikipedia Page: http://en.wikipedia.org/wiki/SHA-1
	 *
	 * Copyright (c) 2012-22 SAURAV MOHAPATRA <mohaps@gmail.com>
	 *
	 * Permission to use, copy, modify, and distribute this software for any
	 * purpose with or without fee is hereby granted, provided that the above
	 * copyright notice and this permission notice appear in all copies.
	 *
	 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
	 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
	 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
	 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
	 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
	 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
	 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
	*/

	class sha1 {
	public:
		using byte = std::uint8_t;
		using digest_t = std::array<std::uint32_t, 5>;

		constexpr sha1& reset() noexcept {
			digest = reset_bytes;
			block_index = 0;
			byte_count = 0;
			dirty = true;
			return *this;
		}

		constexpr sha1& process_byte(byte octet) noexcept {
			dirty = true;

			block[block_index] = octet;
			block_index++;
			byte_count++;
			process_if_needed();
			return *this;
		}

		template<typename T>
			requires std::same_as<char, T> || std::same_as<unsigned char, T>
		constexpr sha1 & process_bytes(const T * start, std::size_t length) noexcept {
			if (length == 0) return *this;

			dirty = true;
			byte_count += length;

			for (std::size_t offset = 0; offset < length;) {
				const auto bytes_left = block_size - block_index;
				const auto to_copy = std::min(length - offset, bytes_left);


				std::copy(start + offset, start + offset + to_copy, block.data() + block_index);

				block_index += to_copy;
				process_if_needed();
				offset += to_copy;
			}

			return *this;
		}

		constexpr sha1& process_nulls(std::size_t count) noexcept {
			if (count == 0) return *this;

			dirty = true;
			byte_count += count;

			for (std::size_t i = 0; i < count;) {
				const auto bytes_left = block_size - block_index;
				const auto to_fill = std::min(count - i, bytes_left);

				std::fill(block.data() + block_index, block.data() + block_index + to_fill, 0);

				block_index += to_fill;
				process_if_needed();
				i += to_fill;
			}

			return *this;
		}

		constexpr sha1& process_bytes(const byte* start, const byte* end) noexcept {
			return process_bytes(start, end - start);
		}

		constexpr sha1& process_bytes(std::span<const byte> sp) noexcept {
			return process_bytes(sp.data(), sp.size());
		}

		constexpr sha1& process_bytes(std::string_view sv) noexcept {
			return process_bytes(sv.data(), sv.length());
		}

		constexpr sha1& process_bytes(const char* str) noexcept {
			return process_bytes(std::string_view{ str });
		}

		template<std::size_t Size>
		constexpr sha1& process_bytes(const std::array<byte, Size>& bytes) noexcept {
			return process_bytes(bytes.data(), bytes.size());
		}

		template<std::size_t Size>
		constexpr sha1& process_bytes(const char(&str)[Size]) {
			return process_bytes(str, (str[Size - 1] == '\0') ? Size - 1 : Size);
		}

		template<typename T>
			requires (std::is_trivially_copyable_v<T>)
		constexpr sha1& process_bytes(const T& type) {
			return process_bytes(std::bit_cast<std::array<byte, sizeof(T)>>(type));
		}

		constexpr digest_t get_digest() noexcept {
			make_digest();
			return digest;
		}

	private:

		constexpr void make_digest() noexcept {
			if (!dirty) return;

			auto bit_count_big_endian = [](auto count) {
				if constexpr (std::endian::native == std::endian::little) {
					return std::byteswap(count);
				}
				else return count;
			}(byte_count * 8);

			process_byte(0x80);

			if (constexpr auto target_index = block_size - sizeof(std::uint64_t); block_index < target_index) {
				process_nulls(target_index - block_index);
			}
			else {
				process_nulls(block_size - block_index + target_index);
			}

			process_bytes(bit_count_big_endian);

			dirty = false;
		}

		constexpr static std::size_t block_size = 64;

		constexpr static std::uint32_t left_rotate(std::uint32_t value, std::size_t count) noexcept {
			return (value << count) ^ (value >> (32 - count));
		}

		constexpr static auto reset_bytes = digest_t{ 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };

		std::array<std::uint8_t, block_size> block{};
		digest_t digest = reset_bytes;
		std::size_t block_index = 0;
		std::size_t byte_count = 0;
		bool dirty = true;

		constexpr void process_if_needed() noexcept {
			if (block_index == block_size) {
				block_index = 0;
				process_block();
			}
		}

		constexpr void process_block() noexcept {
			auto w = std::array<std::uint32_t, 80>{};

			for (std::size_t i = 0; i < 16; i++) {
				w[i] = static_cast<std::uint32_t>(block[i * 4 + 0]) << 24;
				w[i] |= static_cast<std::uint32_t>(block[i * 4 + 1]) << 16;
				w[i] |= static_cast<std::uint32_t>(block[i * 4 + 2]) << 8;
				w[i] |= static_cast<std::uint32_t>(block[i * 4 + 3]);
			}

			for (std::size_t i = 16; i <= 31; i++) {
				w[i] = left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
			}

			for (std::size_t i = 32; i < 80; i++) {
				w[i] = left_rotate(w[i - 6] ^ w[i - 16] ^ w[i - 28] ^ w[i - 32], 2);
			}

			std::uint32_t a = digest[0];
			std::uint32_t b = digest[1];
			std::uint32_t c = digest[2];
			std::uint32_t d = digest[3];
			std::uint32_t e = digest[4];

			for (std::size_t i = 0; i < 80; i++) {
				std::uint32_t f = 0;
				std::uint32_t k = 0;

				if (i < 20) {
					f = (b & c) | (~b & d);
					k = 0x5A827999;
				}
				else if (i < 40) {
					f = b ^ c ^ d;
					k = 0x6ED9EBA1;
				}
				else if (i < 60) {
					f = (b & c) | (b & d) | (c & d);
					k = 0x8F1BBCDC;
				}
				else {
					f = b ^ c ^ d;
					k = 0xCA62C1D6;
				}
				std::uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
				e = d;
				d = c;
				c = left_rotate(b, 30);
				b = a;
				a = temp;
			}

			digest[0] += a;
			digest[1] += b;
			digest[2] += c;
			digest[3] += d;
			digest[4] += e;
		}
	};

	#define STATIC_CHECK_HASH(ct, ...) static_assert(sha1().process_bytes(ct).get_digest() == sha1::digest_t{__VA_ARGS__})
		STATIC_CHECK_HASH("", 0xda39a3ee, 0x5e6b4b0d, 0x3255bfef, 0x95601890, 0xafd80709);
		STATIC_CHECK_HASH("a million bright ambassadors of morning", 0x0014eed1, 0x2309f51f, 0x476e60b0, 0xcf096fcd, 0xfe68d486);
		STATIC_CHECK_HASH("overhead the albatross hangs motionless upon the air", 0x013efc4b, 0x6d93e766, 0x056df3ca, 0x4b36530b, 0xf1413ea7);
	#undef STATIC_CHECK_HASH
}