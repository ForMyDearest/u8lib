#pragma once

#include "msvc_format_ucd_tables.hpp"
#include <u8lib/iterator.hpp>

namespace msvc
{
	// Generated per N4950 [format.string.std]/13, by tools/unicode_properties_parse/format_width_estimate_intervals.cpp
	// in the https://github.com/microsoft/stl repository.
	inline constexpr char32_t _Width_estimate_intervals_v2[] = {
		0x1100u, 0x1160u, 0x231Au, 0x231Cu, 0x2329u, 0x232Bu, 0x23E9u, 0x23EDu, 0x23F0u, 0x23F1u, 0x23F3u, 0x23F4u, 0x25FDu,
		0x25FFu, 0x2614u, 0x2616u, 0x2648u, 0x2654u, 0x267Fu, 0x2680u, 0x2693u, 0x2694u, 0x26A1u, 0x26A2u, 0x26AAu, 0x26ACu,
		0x26BDu, 0x26BFu, 0x26C4u, 0x26C6u, 0x26CEu, 0x26CFu, 0x26D4u, 0x26D5u, 0x26EAu, 0x26EBu, 0x26F2u, 0x26F4u, 0x26F5u,
		0x26F6u, 0x26FAu, 0x26FBu, 0x26FDu, 0x26FEu, 0x2705u, 0x2706u, 0x270Au, 0x270Cu, 0x2728u, 0x2729u, 0x274Cu, 0x274Du,
		0x274Eu, 0x274Fu, 0x2753u, 0x2756u, 0x2757u, 0x2758u, 0x2795u, 0x2798u, 0x27B0u, 0x27B1u, 0x27BFu, 0x27C0u, 0x2B1Bu,
		0x2B1Du, 0x2B50u, 0x2B51u, 0x2B55u, 0x2B56u, 0x2E80u, 0x2E9Au, 0x2E9Bu, 0x2EF4u, 0x2F00u, 0x2FD6u, 0x2FF0u, 0x2FFCu,
		0x3000u, 0x303Fu, 0x3041u, 0x3097u, 0x3099u, 0x3100u, 0x3105u, 0x3130u, 0x3131u, 0x318Fu, 0x3190u, 0x31E4u, 0x31F0u,
		0x321Fu, 0x3220u, 0x3248u, 0x3250u, 0xA48Du, 0xA490u, 0xA4C7u, 0xA960u, 0xA97Du, 0xAC00u, 0xD7A4u, 0xF900u, 0xFB00u,
		0xFE10u, 0xFE1Au, 0xFE30u, 0xFE53u, 0xFE54u, 0xFE67u, 0xFE68u, 0xFE6Cu, 0xFF01u, 0xFF61u, 0xFFE0u, 0xFFE7u,
		0x16FE0u, 0x16FE5u, 0x16FF0u, 0x16FF2u, 0x17000u, 0x187F8u, 0x18800u, 0x18CD6u, 0x18D00u, 0x18D09u, 0x1AFF0u,
		0x1AFF4u, 0x1AFF5u, 0x1AFFCu, 0x1AFFDu, 0x1AFFFu, 0x1B000u, 0x1B123u, 0x1B132u, 0x1B133u, 0x1B150u, 0x1B153u,
		0x1B155u, 0x1B156u, 0x1B164u, 0x1B168u, 0x1B170u, 0x1B2FCu, 0x1F004u, 0x1F005u, 0x1F0CFu, 0x1F0D0u, 0x1F18Eu,
		0x1F18Fu, 0x1F191u, 0x1F19Bu, 0x1F200u, 0x1F203u, 0x1F210u, 0x1F23Cu, 0x1F240u, 0x1F249u, 0x1F250u, 0x1F252u,
		0x1F260u, 0x1F266u, 0x1F300u, 0x1F650u, 0x1F680u, 0x1F6C6u, 0x1F6CCu, 0x1F6CDu, 0x1F6D0u, 0x1F6D3u, 0x1F6D5u,
		0x1F6D8u, 0x1F6DCu, 0x1F6E0u, 0x1F6EBu, 0x1F6EDu, 0x1F6F4u, 0x1F6FDu, 0x1F7E0u, 0x1F7ECu, 0x1F7F0u, 0x1F7F1u,
		0x1F900u, 0x1FA00u, 0x1FA70u, 0x1FA7Du, 0x1FA80u, 0x1FA89u, 0x1FA90u, 0x1FABEu, 0x1FABFu, 0x1FAC6u, 0x1FACEu,
		0x1FADCu, 0x1FAE0u, 0x1FAE9u, 0x1FAF0u, 0x1FAF9u, 0x20000u, 0x2FFFEu, 0x30000u, 0x3FFFEu};

	constexpr int _Unicode_width_estimate(const char32_t _Ch) noexcept {
		// Computes the width estimation for Unicode characters from N4950 [format.string.std]/13
		// The two branches are functionally equivalent; `12` is chosen for performance here.
		constexpr char32_t _Linear_search_threshold = _Width_estimate_intervals_v2[12];
		if (_Ch < _Linear_search_threshold) {
			int _Result = 1;
			for (const auto& _Bound: _Width_estimate_intervals_v2) {
				if (_Ch < _Bound) {
					return _Result;
				}
				_Result ^= 0b11u; // Flip between 1 and 2 on each iteration
			}
			return 1;
		} else {
			const ptrdiff_t _Upper_bound_index =
					std::upper_bound(_Width_estimate_intervals_v2, std::end(_Width_estimate_intervals_v2), _Ch)
					- _Width_estimate_intervals_v2;
			return 1 + (_Upper_bound_index & 1);
		}
	}

	// Implements a DFA matching the regex on the left side of rule GB11. The DFA is:
	//
	// +---+   ExtPic      +---+    ZWJ        +---+
	// | 1 +---------------> 2 +---------------> 3 |
	// +---+               ++-^+               +---+
	//                      | |
	//                      +-+
	//                      Extend
	//
	// Note state 3 is never explicitly transitioned to, since it's the "accept" state, we just
	// transition back to state 1 and return true.
	class _GB11_LeftHand_regex {
	private:
		enum _State_t : bool { _Start, _ExtPic };

		_State_t _State = _Start;

	public:
		constexpr bool operator==(const _GB11_LeftHand_regex&) const noexcept = default;

		constexpr bool _Match(
			const _Grapheme_Break_property_values _Left_gbp, _Extended_Pictographic_property_values _Left_ExtPic) noexcept {
			switch (_State) {
				case _Start:
					if (_Left_ExtPic == _Extended_Pictographic_property_values::_Extended_Pictographic_value) {
						_State = _ExtPic;
					}
					return false;
				case _ExtPic:
					if (_Left_gbp == _Grapheme_Break_property_values::_ZWJ_value) {
						_State = _Start;
						return true;
					} else if (_Left_gbp != _Grapheme_Break_property_values::_Extend_value) {
						_State = _Start;
						return false;
					}
					return false;
				default:
					std::unreachable();
			}
		}
	};

	class _Grapheme_break_property_iterator {
	private:
		using _Wrapped_iter_type = u8lib::unicode_codepoint_iterator;

		_Wrapped_iter_type _WrappedIter;
		_GB11_LeftHand_regex _GB11_rx;

	public:
		constexpr bool operator==(std::default_sentinel_t) const noexcept {
			return _WrappedIter == std::default_sentinel;
		}

		constexpr bool operator==(const _Grapheme_break_property_iterator&) const noexcept = default;

		using difference_type = ptrdiff_t;
		using value_type = std::iter_value_t<_Wrapped_iter_type>;

		constexpr _Grapheme_break_property_iterator(const char8_t* _First, const char8_t* _Last)
			: _WrappedIter(_First, _Last) {}

		constexpr _Grapheme_break_property_iterator() = default;

		constexpr _Grapheme_break_property_iterator& operator++() noexcept {
			auto _Left_gbp = _Grapheme_Break_property_data._Get_property_for_codepoint(*_WrappedIter);
			auto _Left_ExtPic = _Extended_Pictographic_property_data._Get_property_for_codepoint(*_WrappedIter);
			auto _Right_gbp = _Grapheme_Break_property_values::_No_value;
			auto _Right_ExtPic = _Extended_Pictographic_property_values::_No_value;
			size_t _Num_RIs = 0;
			for (;; _Left_gbp = _Right_gbp, _Left_ExtPic = _Right_ExtPic) {
				++_WrappedIter;
				if (_WrappedIter == std::default_sentinel) {
					return *this; // GB2 Any % eot
				}
				_Right_gbp = _Grapheme_Break_property_data._Get_property_for_codepoint(*_WrappedIter);
				_Right_ExtPic = _Extended_Pictographic_property_data._Get_property_for_codepoint(*_WrappedIter);
				// match GB11 now, so that we're sure to update it for every character, not just ones where
				// the GB11 rule is considered
				const bool _GB11_Match = _GB11_rx._Match(_Left_gbp, _Left_ExtPic);
				// Also update the number of sequential RIs immediately
				if (_Left_gbp == _Grapheme_Break_property_values::_Regional_Indicator_value) {
					++_Num_RIs;
				} else {
					_Num_RIs = 0;
				}

				if (_Left_gbp == _Grapheme_Break_property_values::_CR_value
					&& _Right_gbp == _Grapheme_Break_property_values::_LF_value) {
					continue; // GB3 CR x LF
				}

				if (_Left_gbp == _Grapheme_Break_property_values::_Control_value
					|| _Left_gbp == _Grapheme_Break_property_values::_CR_value
					|| _Left_gbp == _Grapheme_Break_property_values::_LF_value) {
					return *this; // GB4 (Control | CR | LF) % Any
				}

				if (_Right_gbp == _Grapheme_Break_property_values::_Control_value
					|| _Right_gbp == _Grapheme_Break_property_values::_CR_value
					|| _Right_gbp == _Grapheme_Break_property_values::_LF_value) {
					return *this; // GB5 Any % (Control | CR | LF)
				}

				if (_Left_gbp == _Grapheme_Break_property_values::_L_value
					&& (_Right_gbp == _Grapheme_Break_property_values::_L_value
						|| _Right_gbp == _Grapheme_Break_property_values::_V_value
						|| _Right_gbp == _Grapheme_Break_property_values::_LV_value
						|| _Right_gbp == _Grapheme_Break_property_values::_LVT_value)) {
					continue; // GB6 L x (L | V | LV | LVT)
				}

				if ((_Left_gbp == _Grapheme_Break_property_values::_LV_value
					|| _Left_gbp == _Grapheme_Break_property_values::_V_value)
					&& (_Right_gbp == _Grapheme_Break_property_values::_V_value
						|| _Right_gbp == _Grapheme_Break_property_values::_T_value)) {
					continue; // GB7 (LV | V) x (V | T)
				}

				if ((_Left_gbp == _Grapheme_Break_property_values::_LVT_value
					|| _Left_gbp == _Grapheme_Break_property_values::_T_value)
					&& _Right_gbp == _Grapheme_Break_property_values::_T_value) {
					continue; // GB8 (LVT | T) x T
				}

				if (_Right_gbp == _Grapheme_Break_property_values::_Extend_value
					|| _Right_gbp == _Grapheme_Break_property_values::_ZWJ_value) {
					continue; // GB9 x (Extend | ZWJ)
				}

				if (_Right_gbp == _Grapheme_Break_property_values::_SpacingMark_value) {
					continue; // GB9a x SpacingMark
				}

				if (_Left_gbp == _Grapheme_Break_property_values::_Prepend_value) {
					continue; // GB9b Prepend x
				}

				if (_GB11_Match && _Right_ExtPic == _Extended_Pictographic_property_values::_Extended_Pictographic_value) {
					continue; // GB11 \p{ExtendedPictographic} Extend* ZWJ x \p{ExtendedPictographic}
				}

				if (_Left_gbp == _Grapheme_Break_property_values::_Regional_Indicator_value
					&& _Right_gbp == _Grapheme_Break_property_values::_Regional_Indicator_value && _Num_RIs % 2 != 0) {
					continue; // GB12 and 13, do not break between RIs if there are an odd number of RIs before the
					// breakpoint
				}
				return *this;
			}
		}

		constexpr _Grapheme_break_property_iterator operator++(int) noexcept {
			auto _Old = *this;
			++*this;
			return _Old;
		}

		constexpr const char8_t* position() const noexcept {
			return _WrappedIter.position();
		}

		constexpr value_type operator*() const noexcept {
			return *_WrappedIter;
		}
	};

	// width iterator for UTF-8 and UTF-16
	class _Measure_string_prefix_iterator_utf {
	private:
		_Grapheme_break_property_iterator _WrappedIter;

	public:
		constexpr bool operator==(const _Measure_string_prefix_iterator_utf&) const noexcept = default;

		constexpr bool operator==(std::default_sentinel_t) const noexcept {
			return _WrappedIter == std::default_sentinel;
		}

		using difference_type = ptrdiff_t;
		using value_type = int;

		constexpr _Measure_string_prefix_iterator_utf(const char8_t* _First, const char8_t* _Last)
			: _WrappedIter(_First, _Last) {}

		constexpr _Measure_string_prefix_iterator_utf() = default;

		constexpr value_type operator*() const {
			return _Unicode_width_estimate(*_WrappedIter);
		}

		constexpr _Measure_string_prefix_iterator_utf& operator++() noexcept {
			++_WrappedIter;
			return *this;
		}

		constexpr _Measure_string_prefix_iterator_utf operator++(int) noexcept {
			auto _Old = *this;
			++*this;
			return _Old;
		}

		constexpr const char8_t* position() const {
			return _WrappedIter.position();
		}
	};

	constexpr bool _Is_printable(char32_t _Val) {
		return __printable_property_data._Get_property_for_codepoint(_Val) == __printable_property_values::_Yes_value;
	}

	constexpr bool _Is_grapheme_extend(char32_t _Val) {
		// TRANSITION, should reuse _Grapheme_Break_property_data to save space.
		// (Grapheme_Extend=Yes is Grapheme_Cluster_Break=Extend minus Emoji_Modifier=Yes.)
		return _Grapheme_Extend_property_data._Get_property_for_codepoint(_Val)
				== _Grapheme_Extend_property_values::_Grapheme_Extend_value;
	}

	template<class _Ty>
	using _Make_standard_integer = std::conditional_t<std::is_signed_v<_Ty>, std::make_signed_t<_Ty>, std::make_unsigned_t<_Ty>>;

	template<class _CharT, std::integral _Ty>
	constexpr bool _In_bounds(const _Ty _Value) {
		return std::in_range<_Make_standard_integer<_CharT>>(static_cast<_Make_standard_integer<_Ty>>(_Value));
	}

	template<std::integral _Integral>
	constexpr std::string_view _Get_integral_prefix(const char _Type, const _Integral _Value) noexcept {
		switch (_Type) {
			case 'b':
				return "0b";
			case 'B':
				return "0B";
			case 'x':
				return "0x";
			case 'X':
				return "0X";
			case 'o':
				if (_Value != _Integral{0}) {
					return "0";
				}
				return {};
			default:
				return {};
		}
	}

	inline int _Count_separators(size_t _Digits, const std::string_view _Groups) {
		if (_Groups.empty()) {
			return 0;
		}

		// Calculate the amount of separators that are going to be inserted based on the groupings of the locale.
		int _Separators = 0;
		auto _Group_it = _Groups.begin();
		while (_Digits > static_cast<size_t>(*_Group_it)) {
			_Digits -= static_cast<size_t>(*_Group_it);
			++_Separators;
			if (_Group_it + 1 != _Groups.end()) {
				++_Group_it;
			}
		}

		return _Separators;
	}

	inline void _Buffer_to_uppercase(char* _First, const char* _Last) noexcept {
		for (; _First != _Last; ++_First) {
			if (*_First >= 'a' && *_First <= 'z') {
				*_First -= 'a' - 'A';
			}
		}
	}

	template<class _CharT, class _OutputIt>
	_OutputIt _Widen_and_copy(const char* _First, const char* const _Last, _OutputIt _Out) {
		for (; _First != _Last; ++_First, (void) ++_Out) {
			*_Out = static_cast<_CharT>(*_First);
		}

		return _Out;
	}
}
