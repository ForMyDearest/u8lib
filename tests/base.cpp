#include <doctest/doctest.h>

#include <u8lib/iterator.hpp>

TEST_CASE("Test Unicode") {
	using namespace u8lib;

	SUBCASE("UTF-8 seq len") {
		const auto u8_len_4 = u8"🐓";
		const auto u8_len_3 = u8"鸡";
		const auto u8_len_2 = u8"Ĝ";
		const auto u8_len_1 = u8"G";

		const auto u16_len_4 = u"🐓";
		const auto u16_len_3 = u"鸡";
		const auto u16_len_2 = u"Ĝ";
		const auto u16_len_1 = u"G";

		const auto u32_len_4 = U"🐓";
		const auto u32_len_3 = U"鸡";
		const auto u32_len_2 = U"Ĝ";
		const auto u32_len_1 = U"G";

		CHECK_EQ(utf8_seq_len(u8_len_4[0]), 4);
		CHECK_EQ(utf8_seq_len(u8_len_3[0]), 3);
		CHECK_EQ(utf8_seq_len(u8_len_2[0]), 2);
		CHECK_EQ(utf8_seq_len(u8_len_1[0]), 1);

		CHECK_EQ(utf8_seq_len(u16_len_4[0]), 4);
		CHECK_EQ(utf8_seq_len(u16_len_3[0]), 3);
		CHECK_EQ(utf8_seq_len(u16_len_2[0]), 2);
		CHECK_EQ(utf8_seq_len(u16_len_1[0]), 1);

		CHECK_EQ(utf8_seq_len(u32_len_4[0]), 4);
		CHECK_EQ(utf8_seq_len(u32_len_3[0]), 3);
		CHECK_EQ(utf8_seq_len(u32_len_2[0]), 2);
		CHECK_EQ(utf8_seq_len(u32_len_1[0]), 1);

		CHECK_EQ(utf8_seq_len(u8_len_4[1]), 0);
		CHECK_EQ(utf8_seq_len(u16_len_4[1]), 4);

		CHECK_EQ(utf8_seq_len(static_cast<char8_t>(0)), 1);
		CHECK_EQ(utf8_seq_len(static_cast<char16_t>(0)), 1);
		CHECK_EQ(utf8_seq_len(static_cast<char32_t>(0)), 1);
	}

	SUBCASE("UTF-16 seq len") {
		const auto u8_len_2 = u8"🐓";
		const auto u8_len_1 = u8"鸡";

		const auto u16_len_2 = u"🐓";
		const auto u16_len_1 = u"鸡";

		const auto u32_len_2 = U"🐓";
		const auto u32_len_1 = U"鸡";

		CHECK_EQ(utf16_seq_len(u8_len_2[0]), 2);
		CHECK_EQ(utf16_seq_len(u8_len_1[0]), 1);

		CHECK_EQ(utf16_seq_len(u16_len_2[0]), 2);
		CHECK_EQ(utf16_seq_len(u16_len_1[0]), 1);

		CHECK_EQ(utf16_seq_len(u32_len_2[0]), 2);
		CHECK_EQ(utf16_seq_len(u32_len_1[0]), 1);

		CHECK_EQ(utf16_seq_len(u8_len_2[1]), 0);
		CHECK_EQ(utf16_seq_len(u16_len_2[1]), 0);

		CHECK_EQ(utf16_seq_len(static_cast<char8_t>(0)), 1);
		CHECK_EQ(utf16_seq_len(static_cast<char16_t>(0)), 1);
		CHECK_EQ(utf16_seq_len(static_cast<char32_t>(0)), 1);
	}

	SUBCASE("UTF-8 index convert") {
		const auto test_str = u8"🐓鸡ĜG";

		// cu => cp, normal
		CHECK_EQ(utf8_code_point_index(test_str, 10, 0), 0);
		CHECK_EQ(utf8_code_point_index(test_str, 10, 1), 0);
		CHECK_EQ(utf8_code_point_index(test_str, 10, 2), 0);
		CHECK_EQ(utf8_code_point_index(test_str, 10, 3), 0);
		CHECK_EQ(utf8_code_point_index(test_str, 10, 4), 1);
		CHECK_EQ(utf8_code_point_index(test_str, 10, 5), 1);
		CHECK_EQ(utf8_code_point_index(test_str, 10, 6), 1);
		CHECK_EQ(utf8_code_point_index(test_str, 10, 7), 2);
		CHECK_EQ(utf8_code_point_index(test_str, 10, 8), 2);
		CHECK_EQ(utf8_code_point_index(test_str, 10, 9), 3);

		// cp => cu, normal
		CHECK_EQ(utf8_code_unit_index(test_str, 10, 0), 0);
		CHECK_EQ(utf8_code_unit_index(test_str, 10, 1), 4);
		CHECK_EQ(utf8_code_unit_index(test_str, 10, 2), 7);
		CHECK_EQ(utf8_code_unit_index(test_str, 10, 3), 9);

		// cu => cp, start with bad ch
		CHECK_EQ(utf8_code_point_index(test_str + 1, 9, 0), 0);
		CHECK_EQ(utf8_code_point_index(test_str + 1, 9, 1), 1);
		CHECK_EQ(utf8_code_point_index(test_str + 1, 9, 2), 2);
		CHECK_EQ(utf8_code_point_index(test_str + 1, 9, 3), 3);
		CHECK_EQ(utf8_code_point_index(test_str + 1, 9, 4), 3);
		CHECK_EQ(utf8_code_point_index(test_str + 1, 9, 5), 3);
		CHECK_EQ(utf8_code_point_index(test_str + 1, 9, 6), 4);
		CHECK_EQ(utf8_code_point_index(test_str + 1, 9, 7), 4);
		CHECK_EQ(utf8_code_point_index(test_str + 1, 9, 8), 5);

		// cp => cu, start with bad ch
		CHECK_EQ(utf8_code_unit_index(test_str + 1, 9, 0), 0);
		CHECK_EQ(utf8_code_unit_index(test_str + 1, 9, 1), 1);
		CHECK_EQ(utf8_code_unit_index(test_str + 1, 9, 2), 2);
		CHECK_EQ(utf8_code_unit_index(test_str + 1, 9, 3), 3);
		CHECK_EQ(utf8_code_unit_index(test_str + 1, 9, 4), 6);
		CHECK_EQ(utf8_code_unit_index(test_str + 1, 9, 5), 8);

		// cu => cp, end with bad ch
		CHECK_EQ(utf8_code_point_index(test_str, 6, 0), 0);
		CHECK_EQ(utf8_code_point_index(test_str, 6, 1), 0);
		CHECK_EQ(utf8_code_point_index(test_str, 6, 2), 0);
		CHECK_EQ(utf8_code_point_index(test_str, 6, 3), 0);
		CHECK_EQ(utf8_code_point_index(test_str, 6, 4), 1);
		CHECK_EQ(utf8_code_point_index(test_str, 6, 5), 2);

		// cp => cu, end with bad ch
		CHECK_EQ(utf8_code_unit_index(test_str, 6, 0), 0);
		CHECK_EQ(utf8_code_unit_index(test_str, 6, 1), 4);
		CHECK_EQ(utf8_code_unit_index(test_str, 6, 2), 5);
	}

	SUBCASE("UTF-16 index convert") {
		const auto test_str = u"🐓鸡ĜG";
		const auto test_str_b = u"🐓🐓🐓";

		// cu => cp, normal
		CHECK_EQ(utf16_code_point_index(test_str, 5, 0), 0);
		CHECK_EQ(utf16_code_point_index(test_str, 5, 1), 0);
		CHECK_EQ(utf16_code_point_index(test_str, 5, 2), 1);
		CHECK_EQ(utf16_code_point_index(test_str, 5, 3), 2);
		CHECK_EQ(utf16_code_point_index(test_str, 5, 4), 3);

		// cp => cu, normal
		CHECK_EQ(utf16_code_unit_index(test_str, 5, 0), 0);
		CHECK_EQ(utf16_code_unit_index(test_str, 5, 1), 2);
		CHECK_EQ(utf16_code_unit_index(test_str, 5, 2), 3);
		CHECK_EQ(utf16_code_unit_index(test_str, 5, 3), 4);

		// cu => cp, start with bad ch
		CHECK_EQ(utf16_code_point_index(test_str + 1, 4, 0), 0);
		CHECK_EQ(utf16_code_point_index(test_str + 1, 4, 1), 1);
		CHECK_EQ(utf16_code_point_index(test_str + 1, 4, 2), 2);
		CHECK_EQ(utf16_code_point_index(test_str + 1, 4, 3), 3);

		// cp => cu, start with bad ch
		CHECK_EQ(utf16_code_unit_index(test_str + 1, 4, 0), 0);
		CHECK_EQ(utf16_code_unit_index(test_str + 1, 4, 1), 1);
		CHECK_EQ(utf16_code_unit_index(test_str + 1, 4, 2), 2);
		CHECK_EQ(utf16_code_unit_index(test_str + 1, 4, 3), 3);

		// cu => cp, end with bad ch
		CHECK_EQ(utf16_code_point_index(test_str_b, 5, 0), 0);
		CHECK_EQ(utf16_code_point_index(test_str_b, 5, 1), 0);
		CHECK_EQ(utf16_code_point_index(test_str_b, 5, 2), 1);
		CHECK_EQ(utf16_code_point_index(test_str_b, 5, 3), 1);
		CHECK_EQ(utf16_code_point_index(test_str_b, 5, 4), 2);

		// cp => cu, end with bad ch
		CHECK_EQ(utf16_code_unit_index(test_str_b, 5, 0), 0);
		CHECK_EQ(utf16_code_unit_index(test_str_b, 5, 1), 2);
		CHECK_EQ(utf16_code_unit_index(test_str_b, 5, 2), 4);
	}

	SUBCASE("UTF-8 parse seq") {
		// parse from different index
		{
			const auto test_str = u8"🐓";

			uint64_t seq_index_0, seq_index_1, seq_index_2, seq_index_3;
			const auto seq_0 = UTF8Seq::ParseUTF8(test_str, 4, 0, seq_index_0);
			const auto seq_1 = UTF8Seq::ParseUTF8(test_str, 4, 1, seq_index_1);
			const auto seq_2 = UTF8Seq::ParseUTF8(test_str, 4, 2, seq_index_2);
			const auto seq_3 = UTF8Seq::ParseUTF8(test_str, 4, 3, seq_index_3);

			CHECK(seq_0.is_valid());
			CHECK(seq_1.is_valid());
			CHECK(seq_2.is_valid());
			CHECK(seq_3.is_valid());

			CHECK_EQ(seq_0, seq_1);
			CHECK_EQ(seq_0, seq_2);
			CHECK_EQ(seq_0, seq_3);

			CHECK_EQ(seq_index_0, 0);
			CHECK_EQ(seq_index_1, 0);
			CHECK_EQ(seq_index_2, 0);
			CHECK_EQ(seq_index_3, 0);
		}

		// bad case
		{
			const auto test_str = u8"鸡🐓";

			uint64_t seq_index_bad_head, seq_index_bad_tail, seq_index_overflow;
			const auto seq_bad_head = UTF8Seq::ParseUTF8(test_str + 1, 6, 0, seq_index_bad_head);
			const auto seq_bad_tail = UTF8Seq::ParseUTF8(test_str, 6, 5, seq_index_bad_tail);
			const auto seq_overflow = UTF8Seq::ParseUTF8(test_str, 6, 6, seq_index_overflow);

			CHECK_FALSE(seq_bad_head.is_valid());
			CHECK_FALSE(seq_bad_tail.is_valid());
			CHECK_FALSE(seq_overflow.is_valid());

			CHECK_EQ(seq_index_bad_head, 0);
			CHECK_EQ(seq_index_bad_tail, 5);
			CHECK_EQ(seq_bad_head.bad_data, test_str[1]);
			CHECK_EQ(seq_bad_tail.bad_data, test_str[5]);
		}
	}

	SUBCASE("UTF-16 parse seq") {
		// parse from different index
		{
			const auto test_str = u"🐓";

			uint64_t seq_index_0, seq_index_1;
			const auto seq_0 = UTF16Seq::ParseUTF16(test_str, 2, 0, seq_index_0);
			const auto seq_1 = UTF16Seq::ParseUTF16(test_str, 2, 1, seq_index_1);

			CHECK(seq_0.is_valid());
			CHECK(seq_1.is_valid());

			CHECK_EQ(seq_0, seq_1);

			CHECK_EQ(seq_index_0, 0);
			CHECK_EQ(seq_index_1, 0);
		}

		// bad case
		{
			const auto test_str = u"G🐓";

			uint64_t seq_index_bad_head, seq_index_bad_tail, seq_index_overflow;
			const auto seq_bad_head = UTF16Seq::ParseUTF16(test_str + 2, 1, 0, seq_index_bad_head);
			const auto seq_bad_tail = UTF16Seq::ParseUTF16(test_str, 2, 1, seq_index_bad_tail);
			const auto seq_overflow = UTF16Seq::ParseUTF16(test_str, 3, 3, seq_index_overflow);

			CHECK_FALSE(seq_bad_head.is_valid());
			CHECK_FALSE(seq_bad_tail.is_valid());
			CHECK_FALSE(seq_overflow.is_valid());

			CHECK_EQ(seq_index_bad_head, 0);
			CHECK_EQ(seq_index_bad_tail, 1);
			CHECK_EQ(seq_bad_head.bad_data, test_str[2]);
			CHECK_EQ(seq_bad_tail.bad_data, test_str[1]);
		}
	}

	SUBCASE("Convert") {
		uint64_t seq_index = 0;

		const auto u8_4 = u8"🐓";
		const auto u8_3 = u8"鸡";
		const auto u8_2 = u8"Ĝ";
		const auto u8_1 = u8"G";

		const auto u8_4_seq = UTF8Seq::ParseUTF8(u8_4, 4, 0, seq_index);
		const auto u8_3_seq = UTF8Seq::ParseUTF8(u8_3, 3, 0, seq_index);
		const auto u8_2_seq = UTF8Seq::ParseUTF8(u8_2, 2, 0, seq_index);
		const auto u8_1_seq = UTF8Seq::ParseUTF8(u8_1, 1, 0, seq_index);

		const auto u16_4 = u"🐓";
		const auto u16_3 = u"鸡";
		const auto u16_2 = u"Ĝ";
		const auto u16_1 = u"G";

		const auto u16_4_seq = UTF16Seq::ParseUTF16(u16_4, 2, 0, seq_index);
		const auto u16_3_seq = UTF16Seq::ParseUTF16(u16_3, 1, 0, seq_index);
		const auto u16_2_seq = UTF16Seq::ParseUTF16(u16_2, 1, 0, seq_index);
		const auto u16_1_seq = UTF16Seq::ParseUTF16(u16_1, 1, 0, seq_index);

		const auto u32_4 = U'🐓';
		const auto u32_3 = U'鸡';
		const auto u32_2 = U'Ĝ';
		const auto u32_1 = U'G';

		CHECK_EQ(UTF16Seq(u8_4_seq), u16_4_seq);
		CHECK_EQ(UTF16Seq(u8_3_seq), u16_3_seq);
		CHECK_EQ(UTF16Seq(u8_2_seq), u16_2_seq);
		CHECK_EQ(UTF16Seq(u8_1_seq), u16_1_seq);

		CHECK_EQ(char32_t(u8_4_seq), u32_4);
		CHECK_EQ(char32_t(u8_3_seq), u32_3);
		CHECK_EQ(char32_t(u8_2_seq), u32_2);
		CHECK_EQ(char32_t(u8_1_seq), u32_1);

		CHECK_EQ(UTF8Seq(u16_4_seq), u8_4_seq);
		CHECK_EQ(UTF8Seq(u16_3_seq), u8_3_seq);
		CHECK_EQ(UTF8Seq(u16_2_seq), u8_2_seq);
		CHECK_EQ(UTF8Seq(u16_1_seq), u8_1_seq);

		CHECK_EQ(char32_t(u16_4_seq), u32_4);
		CHECK_EQ(char32_t(u16_3_seq), u32_3);
		CHECK_EQ(char32_t(u16_2_seq), u32_2);
		CHECK_EQ(char32_t(u16_1_seq), u32_1);

		CHECK_EQ(UTF8Seq(u32_4), u8_4_seq);
		CHECK_EQ(UTF8Seq(u32_3), u8_3_seq);
		CHECK_EQ(UTF8Seq(u32_2), u8_2_seq);
		CHECK_EQ(UTF8Seq(u32_1), u8_1_seq);

		CHECK_EQ(UTF16Seq(u32_4), u16_4_seq);
		CHECK_EQ(UTF16Seq(u32_3), u16_3_seq);
		CHECK_EQ(UTF16Seq(u32_2), u16_2_seq);
		CHECK_EQ(UTF16Seq(u32_1), u16_1_seq);
	}

	SUBCASE("UTF-8 iterator") {
		using Cursor = UTF8Cursor<false>;
		using CCursor = UTF8Cursor<true>;

		// normal str
		{
			const auto test_str = u8"🐓鸡ĜG";
			const UTF8Seq test_seq[] = {
				{u8"🐓", 4},
				{u8"鸡", 3},
				{u8"Ĝ", 2},
				{u8"G", 1}
			};

			uint64_t count;

			// cursor
			count = 0;
			for (auto cursor = CCursor::Begin(test_str, 10); !cursor.reach_end(); cursor.move_next()) {
				CHECK_EQ(cursor.ref(), test_seq[count]);
				++count;
			}
			count = 3;
			for (auto cursor = CCursor::End(test_str, 10); !cursor.reach_begin(); cursor.move_prev()) {
				CHECK_EQ(cursor.ref(), test_seq[count]);
				--count;
			}

			// iter
			count = 0;
			for (auto it = CCursor::Begin(test_str, 10).as_iter(); it.has_next(); it.move_next()) {
				CHECK_EQ(it.ref(), test_seq[count]);
				++count;
			}
			count = 3;
			for (auto it = CCursor::End(test_str, 10).as_iter_inv(); it.has_next(); it.move_next()) {
				CHECK_EQ(it.ref(), test_seq[count]);
				--count;
			}

			// range
			count = 0;
			for (const auto& seq: CCursor::Begin(test_str, 10).as_range()) {
				CHECK_EQ(seq, test_seq[count]);
				++count;
			}
			count = 3;
			for (const auto& seq: CCursor::End(test_str, 10).as_range_inv()) {
				CHECK_EQ(seq, test_seq[count]);
				--count;
			}
		}

		// str with bad ch
		{
			char8_t test_bad_str[10 + 10 + 1];
			char8_t bad_ch = 0b1000'1001;
			UTF8Seq bad_seq = UTF8Seq::Bad(bad_ch);

			// build str
			//     🐓   鸡  Ĝ  G
			// xxx----xx---x--x-xxx
			test_bad_str[0] = bad_ch;
			test_bad_str[1] = bad_ch;
			test_bad_str[2] = bad_ch;
			memcpy(&test_bad_str[3], u8"🐓", 4);
			test_bad_str[7] = bad_ch;
			test_bad_str[8] = bad_ch;
			memcpy(&test_bad_str[9], u8"鸡", 3);
			test_bad_str[12] = bad_ch;
			memcpy(&test_bad_str[13], u8"Ĝ", 2);
			test_bad_str[15] = bad_ch;
			test_bad_str[16] = u8'G';
			test_bad_str[17] = bad_ch;
			test_bad_str[18] = bad_ch;
			test_bad_str[19] = bad_ch;
			test_bad_str[20] = 0;

			// build seq
			const UTF8Seq test_seq[] = {
				bad_seq,
				bad_seq,
				bad_seq,
				{u8"🐓", 4},
				bad_seq,
				bad_seq,
				{u8"鸡", 3},
				bad_seq,
				{u8"Ĝ", 2},
				bad_seq,
				{u8"G", 1},
				bad_seq,
				bad_seq,
				bad_seq
			};

			uint64_t bad_count;

			// cursor
			bad_count = 0;
			for (auto cursor = CCursor::Begin(test_bad_str, 20); !cursor.reach_end(); cursor.move_next()) {
				CHECK_EQ(cursor.ref(), test_seq[bad_count]);
				++bad_count;
			}
			bad_count = 13;
			for (auto cursor = CCursor::End(test_bad_str, 20); !cursor.reach_begin(); cursor.move_prev()) {
				CHECK_EQ(cursor.ref(), test_seq[bad_count]);
				--bad_count;
			}

			// iter
			bad_count = 0;
			for (auto it = CCursor::Begin(test_bad_str, 20).as_iter(); it.has_next(); it.move_next()) {
				CHECK_EQ(it.ref(), test_seq[bad_count]);
				++bad_count;
			}
			bad_count = 13;
			for (auto it = CCursor::End(test_bad_str, 20).as_iter_inv(); it.has_next(); it.move_next()) {
				CHECK_EQ(it.ref(), test_seq[bad_count]);
				--bad_count;
			}

			// range
			bad_count = 0;
			for (const auto& seq: CCursor::Begin(test_bad_str, 20).as_range()) {
				CHECK_EQ(seq, test_seq[bad_count]);
				++bad_count;
			}
			bad_count = 13;
			for (const auto& seq: CCursor::End(test_bad_str, 20).as_range_inv()) {
				CHECK_EQ(seq, test_seq[bad_count]);
				--bad_count;
			}
		}
	}

	SUBCASE("UTF-16 iterator") {
		using Cursor = UTF16Cursor<false>;
		using CCursor = UTF16Cursor<true>;

		// normal str
		{
			const auto test_str = u"🐓鸡ĜG";
			const UTF16Seq test_seq[] = {
				{u"🐓", 2},
				{u"鸡", 1},
				{u"Ĝ", 1},
				{u"G", 1}
			};

			uint64_t count;

			// cursor
			count = 0;
			for (auto cursor = CCursor::Begin(test_str, 5); !cursor.reach_end(); cursor.move_next()) {
				CHECK_EQ(cursor.ref(), test_seq[count]);
				++count;
			}
			count = 3;
			for (auto cursor = CCursor::End(test_str, 5); !cursor.reach_begin(); cursor.move_prev()) {
				CHECK_EQ(cursor.ref(), test_seq[count]);
				--count;
			}

			// iter
			count = 0;
			for (auto it = CCursor::Begin(test_str, 5).as_iter(); it.has_next(); it.move_next()) {
				CHECK_EQ(it.ref(), test_seq[count]);
				++count;
			}
			count = 3;
			for (auto it = CCursor::End(test_str, 5).as_iter_inv(); it.has_next(); it.move_next()) {
				CHECK_EQ(it.ref(), test_seq[count]);
				--count;
			}

			// range
			count = 0;
			for (const auto& seq: CCursor::Begin(test_str, 5).as_range()) {
				CHECK_EQ(seq, test_seq[count]);
				++count;
			}
			count = 3;
			for (const auto& seq: CCursor::End(test_str, 5).as_range_inv()) {
				CHECK_EQ(seq, test_seq[count]);
				--count;
			}
		}

		// str with bad ch
		{
			char16_t test_bad_str[5 + 7 + 1];
			char16_t bad_ch = kUtf16TrailingSurrogateHeader + 10;
			UTF16Seq bad_seq = UTF16Seq::Bad(bad_ch);

			// build str
			//   🐓鸡 Ĝ G
			// xx--x-x-x-xx
			test_bad_str[0] = bad_ch;
			test_bad_str[1] = bad_ch;
			memcpy(&test_bad_str[2], u"🐓", 2 * 2);
			test_bad_str[4] = bad_ch;
			test_bad_str[5] = u'鸡';
			test_bad_str[6] = bad_ch;
			test_bad_str[7] = u'Ĝ';
			test_bad_str[8] = bad_ch;
			test_bad_str[9] = u'G';
			test_bad_str[10] = bad_ch;
			test_bad_str[11] = bad_ch;
			test_bad_str[12] = 0;

			// build seq
			const UTF16Seq test_seq[] = {
				bad_seq,
				bad_seq,
				{u"🐓", 2},
				bad_seq,
				{u"鸡", 1},
				bad_seq,
				{u"Ĝ", 1},
				bad_seq,
				{u"G", 1},
				bad_seq,
				bad_seq
			};

			uint64_t bad_count;

			// cursor
			bad_count = 0;
			for (auto cursor = CCursor::Begin(test_bad_str, 12); !cursor.reach_end(); cursor.move_next()) {
				CHECK_EQ(cursor.ref(), test_seq[bad_count]);
				++bad_count;
			}
			bad_count = 10;
			for (auto cursor = CCursor::End(test_bad_str, 12); !cursor.reach_begin(); cursor.move_prev()) {
				CHECK_EQ(cursor.ref(), test_seq[bad_count]);
				--bad_count;
			}

			// iter
			bad_count = 0;
			for (auto it = CCursor::Begin(test_bad_str, 12).as_iter(); it.has_next(); it.move_next()) {
				CHECK_EQ(it.ref(), test_seq[bad_count]);
				++bad_count;
			}
			bad_count = 10;
			for (auto it = CCursor::End(test_bad_str, 12).as_iter_inv(); it.has_next(); it.move_next()) {
				CHECK_EQ(it.ref(), test_seq[bad_count]);
				--bad_count;
			}

			// range
			bad_count = 0;
			for (const auto& seq: CCursor::Begin(test_bad_str, 12).as_range()) {
				CHECK_EQ(seq, test_seq[bad_count]);
				++bad_count;
			}
			bad_count = 10;
			for (const auto& seq: CCursor::End(test_bad_str, 12).as_range_inv()) {
				CHECK_EQ(seq, test_seq[bad_count]);
				--bad_count;
			}
		}
	}
}
