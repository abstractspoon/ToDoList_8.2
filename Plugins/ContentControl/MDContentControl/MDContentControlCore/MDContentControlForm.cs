﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Diagnostics;
using System.Windows.Forms;
using System.IO;

using Abstractspoon.Tdl.PluginHelpers;
using Command.Handling;

using Markdig;
using Markdig.Syntax;
using Markdig.Syntax.Inlines;
using Markdig.Renderers;

namespace MDContentControl
{
	public partial class MDContentControlForm : UserControl
	{
		public event EventHandler InputTextChanged;
		public event EventHandler InputLostFocus;

		public event LinkClickedEventHandler LinkClicked;

		// -----------------------------------------------------------------

		bool m_RestoreInputFocusAfterUpdate = false;
		bool m_SettingTextOrFont = false;

		// -----------------------------------------------------------------

		public MDContentControlForm()
		{
			InitializeComponent();

			// This line is essential to high DPI system but
			// the form editor keeps removing it if I put it
			// in the form designer.cs file
			SplitContainer.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;

			Win32.SetEditMargins(InputTextCtrl.Handle, DPIScaling.Scale(4));
			Win32.RemoveClientEdge(InputTextCtrl.Handle);
			Win32.AddBorder(InputTextCtrl.Handle);
			Win32.AddBorder(PreviewBrowser.Handle);

			InputTextCtrl.TextChanged += (s, e) =>
			{
				InputTextChanged?.Invoke(this, e);
			};
			
			InputTextCtrl.LostFocus += (s, e) =>
			{
				InputLostFocus?.Invoke(this, e);
			};

			InputTextCtrl.LinkClicked += (s, e) =>
			{
				LinkClicked?.Invoke(this, e);
			};
		}

		public static string ConvertToHtml(Byte[] content)
		{
			if (content.Length == 0)
				return string.Empty;

			var inputText = System.Text.Encoding.Unicode.GetString(content);

			return ConvertToHtml(inputText);
		}

		public static string ConvertToHtml(string content)
		{
			if (content.Length == 0)
				return string.Empty;

			MarkdownPipeline pipeline = new MarkdownPipelineBuilder().UseAdvancedExtensions().Build();
			MarkdownDocument document = Markdown.Parse(content, pipeline);

			foreach (LinkInline link in document.Descendants().OfType<LinkInline>())
			{
				System.Uri uri = null;

				if (System.Uri.TryCreate(link.Url, UriKind.RelativeOrAbsolute, out uri) && !uri.IsAbsoluteUri)
				{
					// Assume it's a local path
					var path = Path.Combine(Directory.GetCurrentDirectory(), link.Url);
					link.Url = new Uri(path).AbsoluteUri;
				}
			}

			var writer = new StringWriter();
			var renderer = new HtmlRenderer(writer);

			pipeline.Setup(renderer);
			renderer.Render(document);

			return writer.ToString();
		}

		public string OutputHtml
		{
			get
			{
				return ConvertToHtml(InputText);
			}
		}

		private string OutputHtmlAsPage
		{
			get
			{
				return "<html>" + m_Style + "<body>" + OutputHtml + "</body></html>";
			}
		}

		public string OutputText
		{
			get
			{
				return PreviewBrowser.Document.Body?.InnerText ?? String.Empty;
			}
		}

		public string InputText
		{
			get
			{
				return InputTextCtrl.Text ?? String.Empty;
			}

			set
			{
				m_SettingTextOrFont = true;

				InputTextCtrl.Text = value;

				m_SettingTextOrFont = false;
			}
		}

		public void SetInputFont(string fontName, int pointSize)
		{
			m_SettingTextOrFont = true;

			InputTextCtrl.Font = new System.Drawing.Font(fontName, pointSize);

			m_SettingTextOrFont = false;
		}

		public bool InsertTextContent(String content, bool bAtEnd)
		{
			if (ReadOnly || !InputTextCtrl.Enabled)
				return false;

			m_SettingTextOrFont = true;

			if (bAtEnd)
				InputText += content;

			// else
			InputTextCtrl.SelectedText = content;

			m_SettingTextOrFont = false;
			return true;
		}

		public bool ReadOnly
		{
			get
			{
				return InputTextCtrl.ReadOnly;
			}

			set
			{
				InputTextCtrl.ReadOnly = value;
				InputTextCtrl.BackColor = (value ? SystemColors.ButtonFace : SystemColors.Window);

				if (PreviewBrowser.Document != null)
					PreviewBrowser.Document.BackColor = (InputTextCtrl.ReadOnly ? SystemColors.ButtonFace : SystemColors.Window);

				// Hide the edit pane if readonly
				SplitContainer.Panel1Collapsed = ReadOnly;
			}
		}

		// We delay the very first update until after 'about:blank'
		// has finished loading else we get visual artifacts because
		// we are eating OnPaintBackground
		static bool m_Initialised = false;

		private void UpdateOutput(bool restoreInputFocus)
		{
			if (!m_Initialised)
				return;

			m_RestoreInputFocusAfterUpdate = restoreInputFocus;

			if (PreviewBrowser.Document != null)
				PreviewBrowser.DocumentText = OutputHtmlAsPage;
			else
				Debug.Assert(false);
		}

		private void HtmlPreview_DocumentCompleted(object sender, WebBrowserDocumentCompletedEventArgs e)
		{
			if (!m_Initialised)
			{
				Debug.Assert(e.Url.ToString() == "about:blank");

				m_Initialised = true;
				UpdateOutput(InputTextCtrl.Focused);
			}

			if (PreviewBrowser.Document != null)
				PreviewBrowser.Document.BackColor = (InputTextCtrl.ReadOnly ? SystemColors.ButtonFace : SystemColors.Window);

			if (m_RestoreInputFocusAfterUpdate)
			{
				InputTextCtrl.Focus();
				m_RestoreInputFocusAfterUpdate = false;
			}
		}

		private void textBox1_TextChanged(object sender, EventArgs e)
		{
			// We don't restore the focus to the input control
			// if the origin of the text change was external
			UpdateOutput(!m_SettingTextOrFont);
		}

		protected override void OnPaintBackground(PaintEventArgs e)
		{
			//base.OnPaintBackground(e);
		}

		public Color SplitBarColor 
		{
			get
			{
				return SplitContainer.BackColor;
			}

			set
			{
				SplitContainer.BackColor = value;
			}
		}

		public int SplitPos
		{
			get
			{
				if (SplitContainer.Orientation == Orientation.Horizontal)
					return (Height - SplitContainer.SplitterDistance);

				// else
				return (Width - SplitContainer.SplitterDistance);
			}
		}

		public bool SetSplitPos(int pos)
		{
			var splitDist = 0;

			if (SplitContainer.Orientation == Orientation.Horizontal)
			{
				splitDist = (Height - pos);

				if (splitDist < SplitContainer.Panel1MinSize)
					return false;

				if (splitDist > (SplitContainer.Height - SplitContainer.Panel2MinSize))
					return false;
			}
			else
			{
				splitDist = (Width - pos);

				if (splitDist < SplitContainer.Panel1MinSize)
					return false;

				if (splitDist > (SplitContainer.Width - SplitContainer.Panel2MinSize))
					return false;
			}

			SplitContainer.SplitterDistance = splitDist;
			return true;
		}

		private readonly string m_BaseStyle = 
			"<style>html,body,table{{font: normal {1}pt {0};}}li{{margin-bottom:7px;}}" +
			"h1{{font-size:{4}pt;}}h2{{font-size:{3}pt;}}h3{{font-size:{2}pt;}}</style>";

		private string m_Style;

		public void SetPreviewFont(string name, int pointSize)
		{
			var newStyle = string.Format(m_BaseStyle,
										name,
										pointSize,      // default
										pointSize + 2,  // H3
										pointSize + 4,  // H2
										pointSize + 8);	// H1

			if (!newStyle.Equals(m_Style))
			{
				m_Style = newStyle;
				UpdateOutput(InputTextCtrl.Focused);
			}
		}

		public bool Undo()
		{
			if (!InputTextCtrl.CanUndo)
				return false;

			// else 
			InputTextCtrl.Undo();
			return true;
		}

		public bool Redo()
		{
			if (!InputTextCtrl.CanRedo)
				return false;

			// else 
			InputTextCtrl.Redo();
			return true;
		}

		private void deleteToolStripMenuItem_Click(object sender, EventArgs e)
		{
			InputTextCtrl.SelectedText = string.Empty;
		}

		private void copyToolStripMenuItem_Click(object sender, EventArgs e)
		{
			InputTextCtrl.Copy();
		}

		private void cutToolStripMenuItem_Click(object sender, EventArgs e)
		{
			InputTextCtrl.Cut();
		}

		private void selectAllToolStripMenuItem_Click(object sender, EventArgs e)
		{
			InputTextCtrl.SelectAll();
		}

		private void pasteToolStripMenuItem_Click(object sender, EventArgs e)
		{
			InputTextCtrl.Paste(DataFormats.GetFormat(DataFormats.Text));
		}

		public bool ProcessMenuShortcut(UInt32 keypress)
		{
			// Return false for deleting to allow default handling
			if (CommandHandling.GetMenuShortcutFromVirtualKey(keypress) == Keys.Delete)
				return false;

			return CommandHandling.ProcessMenuShortcut(keypress, contextMenuStrip1.Items);
		}

		private void contextMenuStrip1_Opening(object sender, CancelEventArgs e)
		{
			// Set the enabled state of the menu items
			bool hasSelection = !String.IsNullOrEmpty(InputTextCtrl.SelectedText);
			bool enabled = (InputTextCtrl.Enabled && !InputTextCtrl.ReadOnly);
			bool hasText = !string.IsNullOrEmpty(InputText);

			CommandHandling.EnableCommand("cutToolStripMenuItem", enabled && hasSelection, contextMenuStrip1.Items);
			CommandHandling.EnableCommand("copyToolStripMenuItem", hasSelection, contextMenuStrip1.Items);
			CommandHandling.EnableCommand("deleteToolStripMenuItem", enabled && hasText, contextMenuStrip1.Items);
			CommandHandling.EnableCommand("pasteToolStripMenuItem", InputTextCtrl.CanPaste(DataFormats.GetFormat(DataFormats.Text)), contextMenuStrip1.Items);
		}

		private void InputTextCtrl_MouseDown(object sender, MouseEventArgs e)
		{
			if (e.Button == MouseButtons.Right)
			{
				// Move the Caret to the click location unless
				// the position is already over some selected text
				int selStart = InputTextCtrl.SelectionStart;
				int selEnd = InputTextCtrl.SelectionStart + InputTextCtrl.SelectionLength;

				int charAtPos = InputTextCtrl.GetCharIndexFromPosition(e.Location);

				if (charAtPos < selStart || charAtPos > selEnd)
				{
					InputTextCtrl.SelectionStart = charAtPos;
					InputTextCtrl.SelectionLength = 0;
				}
			}
		}

		protected override void OnGotFocus(EventArgs e)
		{
			InputTextCtrl.Focus();
		}

		protected override void OnResize(EventArgs e)
		{
			base.OnResize(e);

			// Adjust the splitter orientation to match the aspect ratio of the available space
			if (ClientRectangle.Width > ClientRectangle.Height)
			{
				SplitContainer.Orientation = Orientation.Vertical;
			}
			else if (ClientRectangle.Height > ClientRectangle.Width)
			{
				SplitContainer.Orientation = Orientation.Horizontal;
			}
		}

	}

	// -----------------------------------------------------------------------

	public class RichTextBoxEx : RichTextBox
	{
		protected override CreateParams CreateParams
		{
			get
			{
				CreateParams cp = base.CreateParams;
				cp.ClassName = "RichEdit50W";

				return cp;
			}
		}
	}

}
