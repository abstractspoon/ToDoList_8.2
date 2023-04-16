using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace MarkdownLog
{
    public static class StringExtensions
    {
        public static string Indent(this string text, int indentSize)
        {
            return text.PrependAllLines(new string(' ', indentSize));
        }

        public static string IndentAllExceptFirst(this string text, int indentSize)
        {
            return text.PrependLines(new string(' ', indentSize), 1);
        }

        public static string PrependAllLines(this string text, string prefix)
        {
            return text.PrependLines(prefix);
        }

        private static string PrependLines(this string text, string prefix, int numberToSkip = 0)
        {
            var lines = text.SplitByLine();
            var prependedLines = lines.Skip(numberToSkip).Select(i => prefix + i);
            var prefixedText = string.Join(Environment.NewLine, lines.Take(numberToSkip).Concat(prependedLines));
            return prefixedText;
        }

        public static IList<string> SplitByLine(this string text)
        {
            return text.Split(new[] {"\r\n", "\n\r", "\n", "\r"}, StringSplitOptions.None);
        }

        public static string WrapAt(this string text, int maxCharsPerLine)
        {
            var words = text.Split(' ');
            var sb = new StringBuilder();
            var charsPerLine = 0;
            foreach (string word in words)
            {
                if (charsPerLine + word.Length < maxCharsPerLine)
                {
                    sb.Append(word);

                    if (word != words.Last())
                    {
                        sb.Append(" ");
                    }
                    charsPerLine += word.Length + 1;
                }
                else
                {
                    sb.Append(Environment.NewLine + word + " ");
                    charsPerLine = word.Length + 1;
                }
            }

            return sb.ToString();
        }

        public static string EscapeMarkdownCharacters(this string text)
        {
            var escapedText = text
                .Replace(@"\", @"\\")
                .Replace("`", @"\`")
                .Replace("*", @"\*")
                .Replace("_", @"\_");

            if (escapedText.StartsWith("#"))
            {
                escapedText = @"\" + escapedText;
            }
            else
            {
                escapedText = Regex.Replace(escapedText, @"(?<Number>[0-9]+)\. ", @"${Number}\. ");
            }

            return escapedText;
        }

        public static string Align(this string text, TableColumnAlignment alignment, int width)
        {
            switch (alignment)
            {
                case TableColumnAlignment.Center:
                    var leftPad = Math.Max(0, (width - text.Length) / 2);
                    var rightPad = Math.Max(0, width - text.Length - leftPad);
                    var paddedString = string.Format("{0}{1}{2}", new String(' ', leftPad), text, new String(' ', rightPad));
                    return paddedString;
                case TableColumnAlignment.Right:
                    return text.PadLeft(width);
                case TableColumnAlignment.Left:
                case TableColumnAlignment.Unspecified:
                default:
                    return text.PadRight(width);
            }
        }

        public static string EscapeCSharpString(this string text)
        {
            return text
                .Replace(@"\", @"\\")
                .Replace("\n", @"\n")
                .Replace("\r", @"\r")
                .Replace("\t", @"\t")
                .Replace("\f", @"\f")
                .Replace("\0", @"\0")
                .Replace("\a", @"\a")
                .Replace("\b", @"\b");
        }
    }
}