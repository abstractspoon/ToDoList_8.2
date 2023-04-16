﻿/*****************************************************************************
 * 
 * ReoGrid - .NET Spreadsheet Control
 * 
 * http://reogrid.net/
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * Author: Jing <lujing at unvell.com>
 *
 * Copyright (c) 2012-2016 Jing <lujing at unvell.com>
 * Copyright (c) 2012-2016 unvell.com, all rights reserved.
 * 
 ****************************************************************************/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using RGFloat = System.Single;

namespace unvell.ReoGrid.Drawing.Text
{
	[Obsolete("use FontStyles instead")]
	public enum TextStyle
	{
	}

	/// <summary>
	/// Font style
	/// </summary>
	public enum FontStyles : byte
	{
		/// <summary>
		/// Regular
		/// </summary>
		Regular = 0,

		/// <summary>
		/// Bold
		/// </summary>
		Bold = 1,

		/// <summary>
		/// Italic
		/// </summary>
		Italic = 2,

		/// <summary>
		/// Underline
		/// </summary>
		Underline = 4,

		/// <summary>
		/// Strikethrough
		/// </summary>
		Strikethrough = 8,

		/// <summary>
		/// Superscript
		/// </summary>
		Superscrit = 0x10, 

		/// <summary>
		/// Subscript
		/// </summary>
		Subscript = 0x20,
	}

	internal interface IFont
	{
		string Name { get; set; }
		RGFloat Size { get; set; }
		FontStyles FontStyle { get; set; }
	}

	abstract class BaseFont : IFont
	{
		public string Name { get; set; }

		public RGFloat Size { get; set; }

		public FontStyles FontStyle { get; set; }
	}

}
