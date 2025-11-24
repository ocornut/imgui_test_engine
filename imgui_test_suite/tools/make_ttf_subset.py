import os

from fontTools import subset  # pip install fonttools
from fontTools.ttLib import TTFont


def create_font_subset(input_font_path, output_font_path, extra_chars="", include_ranges=None):
    """
    Create a font subset
    
    Args:
        input_font_path: Path to the input font file
        output_font_path: Path to the output font file
        extra_chars: Additional characters to include (as string)
        include_ranges: List of Unicode ranges to include, defaults to basic character regions
    """

    # Default Unicode ranges to include (Basic Latin characters, numbers, punctuation, etc.)
    if include_ranges is None:
        include_ranges = [
            (0x0020, 0x007F),  # Basic Latin characters
            (0x00A0, 0x00FF),  # Latin-1 Supplement
            (0x0100, 0x017F),  # Latin Extended-A
            (0x2000, 0x206F),  # General Punctuation
            (0x3000, 0x303F),  # CJK Symbols and Punctuation
        ]

    # Build Unicode parameters
    unicode_ranges = []
    for start, end in include_ranges:
        unicode_ranges.append(f"U+{start:04X}-{end:04X}")

    # Add extra characters
    if extra_chars:
        for char in extra_chars:
            code_point = ord(char)
            unicode_ranges.append(f"U+{code_point:04X}")

    unicode_param = ",".join(unicode_ranges)

    # Set subsetting options
    options = subset.Options()
    options.text = None
    options.unicodes = unicode_param
    options.passthrough_tables = True
    options.ignore_missing_unicodes = True
    options.ignore_missing_glyphs = True

    # Execute subsetting
    try:
        subset.main(
            [
                input_font_path,
                f"--output-file={output_font_path}",
                f"--unicodes={unicode_param}",
                "--passthrough-tables",
                "--ignore-missing-unicodes",
                "--ignore-missing-glyphs",
            ]
        )
        print(f"Successfully created font subset: {output_font_path}")
    except Exception as err:
        print(f"Failed to create font subset: {err}")
        return False
    else:
        return True


# Usage example
if __name__ == "__main__":
    # Example 1: Basic usage
    create_font_subset(
        input_font_path="SourceHanSansCN-VF.ttf",
        output_font_path="SourceHanSansCN-VF-subset.ttf",
        extra_chars="零一二三四五六七八九十百千万" + "¢°′″‰℃、。｡､￠，．：；？！％・･ゝゞヽヾーァィゥェォッャュョヮヵヶぁぃぅぇぉっゃゅょゎゕゖㇰㇱㇲㇳㇴㇵㇶㇷㇸㇹㇺㇻㇼㇽㇾㇿ々〻ｧｨｩｪｫｬｭｮｯｰ”〉》」】〕）］｝｣‘“〈《「『【〔（［｛｢£¥＄￡￥＋"
    )
