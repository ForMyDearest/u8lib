#include <doctest/doctest.h>
#include <u8lib/string_view.hpp>

TEST_CASE("Test u8string_view") {
	using namespace u8lib;

	SUBCASE("ctor & dtor") {
		u8string_view empty;
		CHECK_EQ(empty.data(), nullptr);
		CHECK_EQ(empty.size(), 0);
		CHECK_EQ(empty.length(), 0);
		CHECK_EQ(empty.text_length(), 0);

		constexpr char8_t text[] = u8"🐓鸡ĜG";
		u8string_view auto_len{text};
		CHECK_EQ(auto_len.data(), text);
		CHECK_EQ(auto_len.size(), 10);
		CHECK_EQ(auto_len.length(), 10);
		CHECK_EQ(auto_len.text_length(), 4);

		u8string_view manual_len{text, 9};
		CHECK_EQ(manual_len.data(), text);
		CHECK_EQ(manual_len.size(), 9);
		CHECK_EQ(manual_len.length(), 9);
		CHECK_EQ(manual_len.text_length(), 3);
	}

	// [needn't test] copy & move
	// [needn't test] assign & move assign

	SUBCASE("compare") {
		constexpr char8_t text_a[] = u8"🐓鸡ĜG";
		constexpr char8_t text_b[] = u8"🐓鸡Ĝ";
		constexpr char8_t text_c[] = u8"GĜ鸡🐓";

		u8string_view a{text_a};     // 🐓鸡ĜG
		u8string_view b{text_a, 9};  // 🐓鸡Ĝ
		u8string_view c{text_b};     // 🐓鸡Ĝ
		u8string_view d{text_a};     // 🐓鸡ĜG
		u8string_view e{text_c + 6}; // 🐓
		u8string_view f{text_a, 4};  // 🐓
		u8string_view empty{};
		u8string_view with_value_empty{text_a, 0};

		CHECK_EQ(a, a);
		CHECK_EQ(a, d);
		CHECK_EQ(b, c);
		CHECK_EQ(e, f);

		CHECK_NE(a, b);
		CHECK_NE(a, c);
		CHECK_NE(a, e);
		CHECK_NE(a, f);

		CHECK_NE(a, empty);
		CHECK_NE(a, with_value_empty);
		CHECK_EQ(empty, with_value_empty);
	}

	// [needn't test] getter

	SUBCASE("str get") {
		constexpr char8_t text[] = u8"🐓鸡ĜG";
		u8string_view a{text};
		u8string_view b{text, 6};
		u8string_view c{text + 1, 6};

		CHECK_EQ(a.length(), 10);
		CHECK_EQ(a.text_length(), 4);

		CHECK_EQ(b.length(), 6);
		CHECK_EQ(b.text_length(), 3); // 2 bad ch in tail

		CHECK_EQ(c.length(), 6);
		CHECK_EQ(c.text_length(), 4); // 3 bad ch in head
	}

	// [needn't test] validator

	SUBCASE("index & modify") {
		constexpr char8_t text[] = u8"🐓鸡ĜG";
		constexpr UTF8Seq seq[] = {
			{u8"🐓", 4},
			{u8"鸡", 3},
			{u8"Ĝ", 2},
			{u8"G", 1},
		};

		u8string_view view{text};

		CHECK_EQ(view.at(0), text[0]);
		CHECK_EQ(view.at(1), text[1]);
		CHECK_EQ(view.at(2), text[2]);
		CHECK_EQ(view.at(3), text[3]);
		CHECK_EQ(view.at(4), text[4]);
		CHECK_EQ(view.at(5), text[5]);
		CHECK_EQ(view.at(6), text[6]);
		CHECK_EQ(view.at(7), text[7]);
		CHECK_EQ(view.at(8), text[8]);
		CHECK_EQ(view.at(9), text[9]);

		CHECK_EQ(view.at_text(0), seq[0]);
		CHECK_EQ(view.at_text(1), seq[0]);
		CHECK_EQ(view.at_text(2), seq[0]);
		CHECK_EQ(view.at_text(3), seq[0]);
		CHECK_EQ(view.at_text(4), seq[1]);
		CHECK_EQ(view.at_text(5), seq[1]);
		CHECK_EQ(view.at_text(6), seq[1]);
		CHECK_EQ(view.at_text(7), seq[2]);
		CHECK_EQ(view.at_text(8), seq[2]);
		CHECK_EQ(view.at_text(9), seq[3]);

		CHECK_EQ(view.last_text(0), seq[3]);
		CHECK_EQ(view.last_text(1), seq[2]);
		CHECK_EQ(view.last_text(2), seq[2]);
		CHECK_EQ(view.last_text(3), seq[1]);
		CHECK_EQ(view.last_text(4), seq[1]);
		CHECK_EQ(view.last_text(5), seq[1]);
		CHECK_EQ(view.last_text(6), seq[0]);
		CHECK_EQ(view.last_text(7), seq[0]);
		CHECK_EQ(view.last_text(8), seq[0]);
		CHECK_EQ(view.last_text(9), seq[0]);
	}

	SUBCASE("sub view") {
		u8string_view view{u8"🐓鸡ĜG"};
		u8string_view sub_a = view.first_view(4);
		u8string_view sub_b = view.subview(0, 4);
		u8string_view sub_c = view.last_view(6);
		u8string_view sub_d = view.subview(4);
		u8string_view sub_e = view.subview(4, 3);

		CHECK_EQ(sub_a, sub_b);
		CHECK_EQ(sub_a, u8string_view{ u8"🐓" });
		CHECK_EQ(sub_c, sub_d);
		CHECK_EQ(sub_c, u8string_view{ u8"鸡ĜG" });
		CHECK_EQ(sub_e, u8string_view{ u8"鸡" });
	}

	SUBCASE("find") {
		u8string_view view{u8"This 🐓 is 🐓 a good 🐓 text 🐓"};
		u8string_view find_view = u8"🐓";
		UTF8Seq find_seq = {u8"🐓", 4};

		auto found_view = view.find(find_view);
		auto found_seq = view.find(find_seq);
		auto found_last_view = view.rfind(find_view);
		auto found_last_seq = view.rfind(find_seq);

		CHECK(found_view);
		CHECK(found_seq);
		CHECK(found_last_view);
		CHECK(found_last_seq);

		CHECK_EQ(found_view, 5);
		CHECK_EQ(found_seq, 5);
		CHECK_EQ(found_last_view, 35);
		CHECK_EQ(found_last_seq, 35);
	}

	SUBCASE("contains & count") {
		u8string_view view{u8"This 🐓 is 🐓 a good 🐓 text 🐓"};
		u8string_view none_view = u8"🐔";
		UTF8Seq none_seq = {u8"🐔", 4};
		u8string_view has_view = u8"🐓";
		UTF8Seq has_seq = {u8"🐓", 4};

		CHECK_FALSE(view.contains(none_view));
		CHECK_FALSE(view.contains(none_seq));
		CHECK(view.contains(has_view));
		CHECK(view.contains(has_seq));

		CHECK_EQ(view.count(none_view), 0);
		CHECK_EQ(view.count(none_seq), 0);
		CHECK_EQ(view.count(has_view), 4);
		CHECK_EQ(view.count(has_seq), 4);
	}

	SUBCASE("starts_with & ends_with") {
		u8string_view view_a{u8"This is a good text"};
		u8string_view view_b{u8"🐓🐓🐓 This is a good text 🐓🐓🐓"};

		UTF8Seq seq = {u8"🐓", 4};

		CHECK_FALSE(view_a.starts_with(u8"🐓🐓🐓"));
		CHECK_FALSE(view_a.ends_with(u8"🐓🐓🐓"));
		CHECK(view_b.starts_with(u8"🐓🐓🐓"));
		CHECK(view_b.ends_with(u8"🐓🐓🐓"));

		CHECK_FALSE(view_a.starts_with(seq));
		CHECK_FALSE(view_a.ends_with(seq));
		CHECK(view_b.starts_with(seq));
		CHECK(view_b.ends_with(seq));
	}

	SUBCASE("remove prefix & suffix") {
		u8string_view view_a{u8"🐓🐓🐓 This is a good text 🐓🐓🐓"};
		u8string_view view_b{u8" This is a good text 🐓🐓🐓"};
		u8string_view view_c{u8"🐓🐓🐓 This is a good text "};
		u8string_view view_d{u8" This is a good text "};
		u8string_view view_e{u8"🐓🐓 This is a good text 🐓🐓🐓"};
		u8string_view view_f{u8"🐓🐓🐓 This is a good text 🐓🐓"};
		u8string_view view_g{u8"🐓🐓 This is a good text 🐓🐓"};

		UTF8Seq seq = {u8"🐓", 4};

		CHECK_EQ(view_a.RemovePrefix(u8"🐓🐓🐓"), view_b);
		CHECK_EQ(view_a.RemoveSuffix(u8"🐓🐓🐓"), view_c);
		CHECK_EQ(view_a.RemovePrefix(u8" This is a good text"), view_a);
		CHECK_EQ(view_a.RemoveSuffix(u8" This is a good text"), view_a);
		CHECK_EQ(view_g.RemovePrefix(u8"🐓🐓🐓"), view_g);
		CHECK_EQ(view_g.RemoveSuffix(u8"🐓🐓🐓"), view_g);

		CHECK_EQ(view_a.RemovePrefix(seq), view_e);
		CHECK_EQ(view_a.RemoveSuffix(seq), view_f);
		CHECK_EQ(view_d.RemovePrefix(seq), view_d);
		CHECK_EQ(view_d.RemoveSuffix(seq), view_d);
	}

	SUBCASE("trim") {
		u8string_view trim_view{u8" \tThis is a good text\t "};
		u8string_view trim_view_start{u8"This is a good text\t "};
		u8string_view trim_view_end{u8" \tThis is a good text"};
		u8string_view trim_view_full{u8"This is a good text"};
		u8string_view trim_view_start_seq{u8"\tThis is a good text\t "};
		u8string_view trim_view_end_seq{u8" \tThis is a good text\t"};
		u8string_view trim_view_full_seq{u8"\tThis is a good text\t"};
		u8string_view trim_view_empty{u8" \t\t "};
		u8string_view empty{};

		UTF8Seq seq = {u8" ", 1};

		CHECK_EQ(trim_view.trim(), trim_view_full);
		CHECK_EQ(trim_view.trim_start(), trim_view_start);
		CHECK_EQ(trim_view.trim_end(), trim_view_end);
		CHECK_EQ(trim_view.trim(seq), trim_view_full_seq);
		CHECK_EQ(trim_view.trim_start(seq), trim_view_start_seq);
		CHECK_EQ(trim_view.trim_end(seq), trim_view_end_seq);
		CHECK_EQ(trim_view_empty.trim(), empty);

		u8string_view custom_trim_view{u8"🐓🐓🐓 \tThis is a good text\t 🐓🐓🐓"};
		u8string_view custom_trim_view_start{u8"This is a good text\t 🐓🐓🐓"};
		u8string_view custom_trim_view_end{u8"🐓🐓🐓 \tThis is a good text"};
		u8string_view custom_trim_view_full{u8"This is a good text"};
		u8string_view custom_trim_view_start_seq{u8" \tThis is a good text\t 🐓🐓🐓"};
		u8string_view custom_trim_view_end_seq{u8"🐓🐓🐓 \tThis is a good text\t "};
		u8string_view custom_trim_view_full_seq{u8" \tThis is a good text\t "};
		u8string_view custom_trim_view_empty{u8"🐓🐓🐓 \t\t 🐓🐓🐓"};

		u8string_view custom_view = u8"🐓 \t";
		UTF8Seq custom_seq = {u8"🐓", 4};

		CHECK_EQ(custom_trim_view.trim(custom_view), custom_trim_view_full);
		CHECK_EQ(custom_trim_view.trim_start(custom_view), custom_trim_view_start);
		CHECK_EQ(custom_trim_view.trim_end(custom_view), custom_trim_view_end);
		CHECK_EQ(custom_trim_view.trim(custom_seq), custom_trim_view_full_seq);
		CHECK_EQ(custom_trim_view.trim_start(custom_seq), custom_trim_view_start_seq);
		CHECK_EQ(custom_trim_view.trim_end(custom_seq), custom_trim_view_end_seq);
		CHECK_EQ(custom_trim_view_empty.trim(custom_view), empty);
	}

	SUBCASE("trim invalid") {
		u8string_view view{u8"🐓🐓🐓 This is a good text 🐓🐓🐓"};
		u8string_view bad = view.subview(1, view.size() - 2);
		u8string_view trim_start = bad.subview(3);
		u8string_view trim_end = bad.subview(0, bad.size() - 3);
		u8string_view trim_full = u8"🐓🐓 This is a good text 🐓🐓";

		CHECK_EQ(bad.trim_invalid(), trim_full);
		CHECK_EQ(bad.trim_invalid_start(), trim_start);
		CHECK_EQ(bad.trim_invalid_end(), trim_end);
	}

	SUBCASE("partition") {
		SUBCASE("view partition") {
			// util
			u8string_view view{u8"This is a 🐓 good text"};
			u8string_view left{u8"This is a "};
			u8string_view mid{u8"🐓"};
			u8string_view right{u8" good text"};
			auto [result_left, result_mid, result_right] = view.partition(u8"🐓");
			CHECK_EQ(result_left, left);
			CHECK_EQ(result_mid, mid);
			CHECK_EQ(result_right, right);

			// not found
			u8string_view not_found_view{u8"This is a good text"};
			auto [not_found_left, not_found_mid, not_found_right] = not_found_view.partition(u8"🐓");
			CHECK_EQ(not_found_left, not_found_view);
			CHECK(not_found_mid.empty());
			CHECK(not_found_right.empty());

			// first_view
			u8string_view first_view{u8"🐓 This is a good text"};
			auto [first_left, first_mid, first_right] = first_view.partition(u8"🐓");
			CHECK(first_left.empty());
			CHECK_EQ(first_mid, u8"🐓");
			CHECK_EQ(first_right, u8string_view{ u8" This is a good text" });

			// last_view
			u8string_view last_view{u8"This is a good text 🐓"};
			auto [last_left, last_mid, last_right] = last_view.partition(u8"🐓");
			CHECK_EQ(last_left, u8string_view{ u8"This is a good text " });
			CHECK_EQ(last_mid, u8"🐓");
			CHECK(last_right.empty());
		}

		SUBCASE("seq partition") {
			// util
			u8string_view view{u8"This is a 🐓 good text"};
			u8string_view left{u8"This is a "};
			u8string_view mid{u8"🐓"};
			u8string_view right{u8" good text"};
			auto [result_left, result_mid, result_right] = view.partition(UTF8Seq{u8"🐓", 4});
			CHECK_EQ(result_left, left);
			CHECK_EQ(result_mid, mid);
			CHECK_EQ(result_right, right);

			// not found
			u8string_view not_found_view{u8"This is a good text"};
			auto [not_found_left, not_found_mid, not_found_right] = not_found_view.partition(UTF8Seq{u8"🐓", 4});
			CHECK_EQ(not_found_left, not_found_view);
			CHECK(not_found_mid.empty());
			CHECK(not_found_right.empty());

			// first_view
			u8string_view first_view{u8"🐓 This is a good text"};
			auto [first_left, first_mid, first_right] = first_view.partition(UTF8Seq{u8"🐓", 4});
			CHECK(first_left.empty());
			CHECK_EQ(first_mid, u8"🐓");
			CHECK_EQ(first_right, u8string_view{ u8" This is a good text" });

			// last_view
			u8string_view last_view{u8"This is a good text 🐓"};
			auto [last_left, last_mid, last_right] = last_view.partition(UTF8Seq{u8"🐓", 4});
			CHECK_EQ(last_left, u8string_view{ u8"This is a good text " });
			CHECK_EQ(last_mid, u8"🐓");
			CHECK(last_right.empty());
		}
	}

	SUBCASE("split") {
		u8string_view view{u8"This 🐓 is 🐓🐓 a good 🐓 text 🐓"};
		u8string_view split_view{u8"🐓"};
		u8string_view split_result[] = {
			u8"This ",
			u8" is ",
			u8"",
			u8" a good ",
			u8" text ",
		};
		u8string_view split_result_cull_empty[] = {
			u8"This ",
			u8" is ",
			u8" a good ",
			u8" text ",
		};

		SUBCASE("split to container") {
			std::vector<u8string_view> result;
			uint64_t count;
			result.clear();
			count = view.split(result, split_view);
			CHECK_EQ(count, 5);
			for (uint64_t i = 0; i < count; ++i) {
				CHECK_EQ(result[i], split_result[i]);
			}

			result.clear();
			count = view.split(result, split_view, true);
			CHECK_EQ(count, 4);
			for (uint64_t i = 0; i < count; ++i) {
				CHECK_EQ(result[i], split_result_cull_empty[i]);
			}

			result.clear();
			count = view.split(result, split_view, false, 3);
			CHECK_EQ(count, 3);
			for (uint64_t i = 0; i < count; ++i) {
				CHECK_EQ(result[i], split_result[i]);
			}

			result.clear();
			count = view.split(result, split_view, true, 3);
			CHECK_EQ(count, 3);
			for (uint64_t i = 0; i < count; ++i) {
				CHECK_EQ(result[i], split_result_cull_empty[i]);
			}
		}

		SUBCASE("custom split") {
			uint64_t count, idx;

			idx = 0;
			count = view.split_each(
				[&](const u8string_view& v) {
					CHECK_EQ(v, split_result[idx]);
					++idx;
				},
				split_view);
			CHECK_EQ(count, 5);

			idx = 0;
			count = view.split_each(
				[&](const u8string_view& v) {
					CHECK_EQ(v, split_result_cull_empty[idx]);
					++idx;
				},
				split_view,
				true);
			CHECK_EQ(count, 4);

			idx = 0;
			count = view.split_each(
				[&](const u8string_view& v) {
					CHECK_EQ(v, split_result[idx]);
					++idx;
				},
				split_view,
				false,
				3);
			CHECK_EQ(count, 3);

			idx = 0;
			count = view.split_each(
				[&](const u8string_view& v) {
					CHECK_EQ(v, split_result_cull_empty[idx]);
					++idx;
				},
				split_view,
				true,
				3);
			CHECK_EQ(count, 3);
		}
	}

	SUBCASE("text index") {
		u8string_view view{u8"🐓鸡ĜG"};

		CHECK_EQ(view.buffer_index_to_text(0), 0);
		CHECK_EQ(view.buffer_index_to_text(1), 0);
		CHECK_EQ(view.buffer_index_to_text(2), 0);
		CHECK_EQ(view.buffer_index_to_text(3), 0);
		CHECK_EQ(view.buffer_index_to_text(4), 1);
		CHECK_EQ(view.buffer_index_to_text(5), 1);
		CHECK_EQ(view.buffer_index_to_text(6), 1);
		CHECK_EQ(view.buffer_index_to_text(7), 2);
		CHECK_EQ(view.buffer_index_to_text(8), 2);
		CHECK_EQ(view.buffer_index_to_text(9), 3);

		CHECK_EQ(view.text_index_to_buffer(0), 0);
		CHECK_EQ(view.text_index_to_buffer(1), 4);
		CHECK_EQ(view.text_index_to_buffer(2), 7);
		CHECK_EQ(view.text_index_to_buffer(3), 9);

		u8string_view bad_view = view.subview(1, view.size() - 5);

		CHECK_EQ(bad_view.buffer_index_to_text(0), 0);
		CHECK_EQ(bad_view.buffer_index_to_text(1), 1);
		CHECK_EQ(bad_view.buffer_index_to_text(2), 2);
		CHECK_EQ(bad_view.buffer_index_to_text(3), 3);
		CHECK_EQ(bad_view.buffer_index_to_text(4), 4);

		CHECK_EQ(bad_view.text_index_to_buffer(0), 0);
		CHECK_EQ(bad_view.text_index_to_buffer(1), 1);
		CHECK_EQ(bad_view.text_index_to_buffer(2), 2);
		CHECK_EQ(bad_view.text_index_to_buffer(3), 3);
		CHECK_EQ(bad_view.text_index_to_buffer(4), 4);
	}

	SUBCASE("cursor & iterator") {
		u8string_view view{u8"🐓鸡ĜG"};
		UTF8Seq each_result[] = {
			{u8"🐓", 4},
			{u8"鸡", 3},
			{u8"Ĝ", 2},
			{u8"G", 1},
		};

		uint64_t idx;

		SUBCASE("cursor") {
			idx = 0;
			for (auto cursor = view.cursor_begin(); !cursor.reach_end(); cursor.move_next()) {
				CHECK_EQ(cursor.ref(), each_result[idx]);
				++idx;
			}
			CHECK_EQ(idx, 4);

			idx = 4;
			for (auto cursor = view.cursor_end(); !cursor.reach_begin(); cursor.move_prev()) {
				--idx;
				CHECK_EQ(cursor.ref(), each_result[idx]);
			}
			CHECK_EQ(idx, 0);
		}

		SUBCASE("iter") {
			idx = 0;
			for (auto iter = view.iter(); iter.has_next(); iter.move_next()) {
				CHECK_EQ(iter.ref(), each_result[idx]);
				++idx;
			}
			CHECK_EQ(idx, 4);

			idx = 4;
			for (auto iter = view.iter_inv(); iter.has_next(); iter.move_next()) {
				--idx;
				CHECK_EQ(iter.ref(), each_result[idx]);
			}
			CHECK_EQ(idx, 0);
		}

		SUBCASE("range") {
			idx = 0;
			for (auto v: view.range()) {
				CHECK_EQ(v, each_result[idx]);
				++idx;
			}
			CHECK_EQ(idx, 4);

			idx = 4;
			for (auto v: view.range_inv()) {
				--idx;
				CHECK_EQ(v, each_result[idx]);
			}
			CHECK_EQ(idx, 0);
		}
	}
}
