﻿
using System;
using System.Runtime.InteropServices;

using Abstractspoon.Tdl.PluginHelpers;

// PLS DON'T ADD OTHER 'USING' STATEMENTS WHILE I AM STILL LEARNING!

namespace MDContentControl
{
    [System.ComponentModel.DesignerCategory("")]
    public class MDContentControlCore : MarkdownSharpEditorForm, IContentControlWnd
    {
        private IntPtr m_hwndParent;

        public MDContentControlCore(IntPtr hwndParent)
        {
            m_hwndParent = hwndParent;

            InitializeComponent();

            InputText.TextChanged += new System.EventHandler(OnInputTextChanged);
            InputText.LostFocus += new System.EventHandler(OnInputTextLostFocus);

			Win32.AddBorder(HtmlPreview.Handle);
		}

		// ITDLContentControl ------------------------------------------------------------------

		public Byte[] GetContent()
        {
            return System.Text.Encoding.Unicode.GetBytes(InputText.Text);
        }

        public bool SetContent(Byte[] content, bool bResetSelection)
        {
			InputText.Text = System.Text.Encoding.Unicode.GetString(content);
            return true;
        }

        // text content if supported. return false if not supported
        public String GetTextContent()
        {
            return InputText.Text;
        }

        public bool SetTextContent(String content, bool bResetSelection)
        {
			InputText.Text = content;
            return true;
        }

        public bool ProcessMessage(IntPtr hwnd, UInt32 message, UInt32 wParam, UInt32 lParam, UInt32 time, Int32 xPos, Int32 yPos)
        {
            // TODO
            return false;
        }

        public bool Undo()
        {
            // TODO
            return false;
        }

        public bool Redo()
        {
            // TODO
            return false;
        }

        public void SetUITheme(UITheme theme)
        {
			SetSplitBarColor(theme.GetAppDrawingColor(UITheme.AppColor.AppBackDark));


		}

        public void SetReadOnly(bool bReadOnly)
        {
            // TODO

        }

        public void SavePreferences(Preferences prefs, String key)
        {
			prefs.WriteProfileInt(key, "SplitPos", SplitPos);
        }

        public void LoadPreferences(Preferences prefs, String key, bool appOnly)
        {
			SetHtmlFont(prefs.GetProfileString("Preferences", "HtmlFont", "Verdana"),
						prefs.GetProfileInt("Preferences", "HtmlSize", 2));
			
			if (!appOnly)
			{
				SplitPos = prefs.GetProfileInt(key, "SplitPos", ClientSize.Height / 2);
			}
		}

		// --------------------------------------------------------------------

		protected override void OnResize(System.EventArgs e)
        {
            base.OnResize(e);

            Win32.RemoveClientEdge(Handle);
		}

		private void InitializeComponent()
        {
            this.SuspendLayout();
            // 
            // MDContentControlCore
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.ClientSize = new System.Drawing.Size(603, 716);
            this.Name = "MDContentControlCore";
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        private void OnInputTextChanged(object sender, EventArgs e)
        {
			ContentControlWnd.ParentNotify notify = new ContentControlWnd.ParentNotify(m_hwndParent);

            notify.NotifyChange();
        }

        private void OnInputTextLostFocus(object sender, EventArgs e)
        {
			ContentControlWnd.ParentNotify notify = new ContentControlWnd.ParentNotify(m_hwndParent);

            notify.NotifyKillFocus();
        }

    }
}
