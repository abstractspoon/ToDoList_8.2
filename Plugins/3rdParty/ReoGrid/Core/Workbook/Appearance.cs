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

using unvell.ReoGrid.Graphics;
using unvell.ReoGrid.Rendering;
using unvell.ReoGrid.Utility;

namespace unvell.ReoGrid
{
	#region Appearance

	/// <summary>
	/// Key of control appearance item
	/// </summary>
	public enum ControlAppearanceColors : short
	{
#pragma warning disable 1591
		LeadHeadNormal = 1,
		LeadHeadHover = 2,
		LeadHeadSelected = 3,

		LeadHeadIndicatorStart = 11,
		LeadHeadIndicatorEnd = 12,

		ColHeadSplitter = 20,
		ColHeadNormalStart = 21,
		ColHeadNormalEnd = 22,
		ColHeadHoverStart = 23,
		ColHeadHoverEnd = 24,
		ColHeadSelectedStart = 25,
		ColHeadSelectedEnd = 26,
		ColHeadFullSelectedStart = 27,
		ColHeadFullSelectedEnd = 28,
		ColHeadSelectedNotFocusedStart = 29,
		ColHeadSelectedNotFocusedEnd = 30,
		ColHeadFullSelectedNotFocusedStart = 31,
		ColHeadFullSelectedNotFocusedEnd = 32,
		ColHeadInvalidStart = 33,
		ColHeadInvalidEnd = 34,
		ColHeadText = 36,

		RowHeadSplitter = 40,
		RowHeadNormal = 41,
		RowHeadHover = 42,
		RowHeadSelected = 43,
		RowHeadFullSelected = 44,
		RowHeadSelectedNotFocused = 45,
		RowHeadFullSelectedNotFocused = 46,
		RowHeadInvalid = 47,
		RowHeadText = 51,

		SelectionBorder = 61,
		SelectionFill = 62,
		SelectionNotFocusedBorder = 63,
		SelectionNotFocusedFill = 64,

		HotTrackingBackground = 71,
		HotTrackingNotFocusedBackground = 72,

		GridBackground = 81,
		GridText = 82,
		GridLine = 83,

		OutlinePanelBorder = 91,
		OutlinePanelBackground = 92,
		OutlineButtonBorder = 93,
		OutlineButtonText = 94,

		SheetTabBorder = 201,
		SheetTabBackground = 202,
		SheetTabText = 203,
		SheetTabSelected = 204,

#pragma warning restore 1591
	}
		
	/// <summary>
	/// ReoGrid Control Appearance Colors
	/// </summary>
	public class ControlAppearanceStyle
	{
		internal ReoGridControl CurrentControl { get; set; }

		private Dictionary<ControlAppearanceColors, SolidColor> colors = new Dictionary<ControlAppearanceColors, SolidColor>(100);

		internal Dictionary<ControlAppearanceColors, SolidColor> Colors
		{
			get { return colors; }
			set { colors = value; }
		}

		/// <summary>
		/// Get color for appearance item
		/// </summary>
		/// <param name="colorKey">key to get the color item</param>
		/// <param name="color">output color get by specified key</param>
		/// <returns>true if color is found by specified key</returns>
		public bool GetColor(ControlAppearanceColors colorKey, out SolidColor color)
		{
			return colors.TryGetValue(colorKey, out color);
		}

		/// <summary>
		/// Set color for appearance item
		/// </summary>
		/// <param name="colorKey">Key of appearance item</param>
		/// <param name="color">Color to be set</param>
		public void SetColor(ControlAppearanceColors colorKey, SolidColor color)
		{
			colors[colorKey] = color;
			this.CurrentControl?.ApplyControlStyle();
		}

		/// <summary>
		/// Get or set color for appearance items
		/// </summary>
		/// <param name="colorKey"></param>
		/// <returns></returns>
		public SolidColor this[ControlAppearanceColors colorKey]
		{
			get
			{
				SolidColor color;
				if (this.colors.TryGetValue(colorKey, out color))
					return color;
				else
					return SolidColor.Black;
			}
			set
			{
				SetColor(colorKey, value);
			}
		}

		/// <summary>
		/// Try get a color item from control appearance style set
		/// </summary>
		/// <param name="key">Key used to specify a item</param>
		/// <param name="color">Output color struction</param>
		/// <returns>True if key was found and color could be returned; otherwise return false</returns>
		public bool TryGetColor(ControlAppearanceColors key, out SolidColor color)
		{
			return this.colors.TryGetValue(key, out color);
		}

		/// <summary>
		/// Get or set selection border weight
		/// </summary>
		public float SelectionBorderWidth { get; set; }

		/// <summary>
		/// Construct empty control appearance
		/// </summary>
		//[Obsolete("use ControlAppearanceStyle.CreateDefaultControlStyle() to create instance")]
		public ControlAppearanceStyle()
		{
			this.SelectionBorderWidth = 3f;
		}

		/// <summary>
		/// Construct control appearance with two theme colors
		/// </summary>
		/// <param name="mainTheme">Main theme color</param>
		/// <param name="salientTheme">Salient theme color</param>
		/// <param name="useSystemHighlight">Whether use highlight colors of system default</param>
		public ControlAppearanceStyle(SolidColor mainTheme, SolidColor salientTheme, bool useSystemHighlight)
		{
			SolidColor lightMainTheme = ColorUtility.LightColor(mainTheme);
			SolidColor lightLightMainTheme = ColorUtility.LightLightColor(mainTheme);
			SolidColor lightLightLightMainTheme = ColorUtility.LightLightLightColor(mainTheme);
			SolidColor darkMainTheme = ColorUtility.DarkColor(mainTheme);
			SolidColor darkDarkMainTheme = ColorUtility.DarkDarkColor(mainTheme);
			SolidColor lightSalientTheme = ColorUtility.LightColor(salientTheme);
			SolidColor lightLightSalientTheme = ColorUtility.LightLightColor(salientTheme);
			SolidColor lightLightLightSalientTheme = ColorUtility.LightLightLightColor(salientTheme);
			SolidColor darkSalientTheme = ColorUtility.DarkColor(salientTheme);
			SolidColor darkDarkSalientTheme = ColorUtility.DarkDarkColor(salientTheme);

			var backgroundColor = ColorUtility.ChangeColorBrightness(mainTheme, 0.5f);

			SolidColor leadHead = mainTheme;
			colors[ControlAppearanceColors.LeadHeadNormal] = leadHead;
			colors[ControlAppearanceColors.LeadHeadSelected] = darkMainTheme;
			colors[ControlAppearanceColors.LeadHeadIndicatorStart] = lightLightLightSalientTheme;
			colors[ControlAppearanceColors.LeadHeadIndicatorEnd] = lightLightSalientTheme;

			colors[ControlAppearanceColors.ColHeadSplitter] = mainTheme;
			colors[ControlAppearanceColors.ColHeadNormalStart] = lightLightLightMainTheme;
			colors[ControlAppearanceColors.ColHeadNormalEnd] = mainTheme;
			colors[ControlAppearanceColors.ColHeadSelectedStart] = lightLightLightSalientTheme;
			colors[ControlAppearanceColors.ColHeadSelectedEnd] = salientTheme;
			colors[ControlAppearanceColors.ColHeadFullSelectedStart] = lightLightLightSalientTheme;
			colors[ControlAppearanceColors.ColHeadFullSelectedEnd] = lightLightSalientTheme;
			colors[ControlAppearanceColors.ColHeadSelectedNotFocusedStart] = lightLightLightSalientTheme;
			colors[ControlAppearanceColors.ColHeadSelectedNotFocusedEnd] = salientTheme;
			colors[ControlAppearanceColors.ColHeadFullSelectedNotFocusedStart] = lightLightLightSalientTheme;
			colors[ControlAppearanceColors.ColHeadFullSelectedNotFocusedEnd] = lightLightSalientTheme;
			colors[ControlAppearanceColors.ColHeadText] = darkDarkMainTheme;

			colors[ControlAppearanceColors.RowHeadSplitter] = mainTheme;
			colors[ControlAppearanceColors.RowHeadNormal] = lightLightMainTheme;
			colors[ControlAppearanceColors.RowHeadHover] = ColorUtility.DarkColor(leadHead);
			colors[ControlAppearanceColors.RowHeadSelected] = lightSalientTheme;
			colors[ControlAppearanceColors.RowHeadFullSelected] = lightLightSalientTheme;
			colors[ControlAppearanceColors.RowHeadSelectedNotFocused] = lightSalientTheme;
			colors[ControlAppearanceColors.RowHeadFullSelectedNotFocused] = lightLightSalientTheme;
			colors[ControlAppearanceColors.RowHeadText] = darkDarkMainTheme;

			if (useSystemHighlight)
			{
				colors[ControlAppearanceColors.SelectionFill] = new SolidColor(30, StaticResources.SystemColor_Highlight);
				colors[ControlAppearanceColors.SelectionBorder] = new SolidColor(180, StaticResources.SystemColor_Highlight);
				colors[ControlAppearanceColors.SelectionNotFocusedFill] = new SolidColor(30, StaticResources.SystemColor_Control);
				colors[ControlAppearanceColors.SelectionNotFocusedBorder] = new SolidColor(180, StaticResources.SystemColor_Control);
				colors[ControlAppearanceColors.HotTrackingBackground] = new SolidColor(20, StaticResources.SystemColor_Highlight);
				colors[ControlAppearanceColors.HotTrackingNotFocusedBackground] = new SolidColor(20, StaticResources.SystemColor_Control);
			}
			else
			{
				colors[ControlAppearanceColors.SelectionFill] = ColorUtility.FromAlphaColor(30, darkSalientTheme);
				colors[ControlAppearanceColors.SelectionBorder] = ColorUtility.FromAlphaColor(180, lightSalientTheme);
				colors[ControlAppearanceColors.SelectionNotFocusedFill] = ColorUtility.FromAlphaColor(30, darkSalientTheme);
				colors[ControlAppearanceColors.SelectionNotFocusedBorder] = ColorUtility.FromAlphaColor(180, lightSalientTheme);
				colors[ControlAppearanceColors.HotTrackingBackground] = ColorUtility.FromAlphaColor(20, darkSalientTheme);
				colors[ControlAppearanceColors.HotTrackingNotFocusedBackground] = ColorUtility.FromAlphaColor(20, lightSalientTheme);
			}

			colors[ControlAppearanceColors.GridBackground] = backgroundColor;
			colors[ControlAppearanceColors.GridLine] = ColorUtility.ChangeColorBrightness(mainTheme, 0.4f);
			colors[ControlAppearanceColors.GridText] = StaticResources.SystemColor_WindowText;

			colors[ControlAppearanceColors.OutlineButtonBorder] = mainTheme;
			colors[ControlAppearanceColors.OutlinePanelBackground] = lightLightMainTheme;
			colors[ControlAppearanceColors.OutlinePanelBorder] = mainTheme;
			colors[ControlAppearanceColors.OutlineButtonText] = darkSalientTheme;

			colors[ControlAppearanceColors.SheetTabBackground] = lightLightMainTheme;
			colors[ControlAppearanceColors.SheetTabText] = StaticResources.SystemColor_WindowText;
			colors[ControlAppearanceColors.SheetTabBorder] = mainTheme;
			colors[ControlAppearanceColors.SheetTabSelected] = backgroundColor;

			this.SelectionBorderWidth = 3;
		}

		internal SolidColor GetColHeadStartColor(bool isHover, bool isSelected, bool isFullSelected, bool isInvalid, bool isFocused)
		{
			if (isFullSelected)
				return isFocused ?
							colors[ControlAppearanceColors.ColHeadFullSelectedStart] :
							colors[ControlAppearanceColors.ColHeadFullSelectedNotFocusedStart];

			if (isSelected)
				return isFocused ?
							colors[ControlAppearanceColors.ColHeadSelectedStart] :
							colors[ControlAppearanceColors.ColHeadSelectedNotFocusedStart];

			if (isHover)
				return colors[ControlAppearanceColors.ColHeadHoverStart];

			if (isInvalid)
				return colors[ControlAppearanceColors.ColHeadInvalidStart];

			// all else
			return colors[ControlAppearanceColors.ColHeadNormalStart];
		}

		internal SolidColor GetColHeadEndColor(bool isHover, bool isSelected, bool isFullSelected, bool isInvalid, bool isFocused)
		{
			if (isFullSelected)
				return isFocused ? 
							colors[ControlAppearanceColors.ColHeadFullSelectedEnd] :
							colors[ControlAppearanceColors.ColHeadFullSelectedNotFocusedEnd];

			if (isSelected)
				return isFocused ?
							colors[ControlAppearanceColors.ColHeadSelectedEnd] :
							colors[ControlAppearanceColors.ColHeadSelectedNotFocusedEnd];

			if (isHover)
				return colors[ControlAppearanceColors.ColHeadHoverEnd];

			if (isInvalid)
				return colors[ControlAppearanceColors.ColHeadInvalidEnd];

			// all else
			return colors[ControlAppearanceColors.ColHeadNormalEnd];
		}

		internal SolidColor GetRowHeadColor(bool isHover, bool isSelected, bool isFullSelected, bool isInvalid, bool isFocused)
		{
			if (isFullSelected)
				return isFocused ?
							colors[ControlAppearanceColors.RowHeadFullSelected] :
							colors[ControlAppearanceColors.RowHeadFullSelectedNotFocused];

			if (isSelected)
				return isFocused ?
							colors[ControlAppearanceColors.RowHeadSelected] :
							colors[ControlAppearanceColors.RowHeadSelectedNotFocused];

			if (isHover)
				return colors[ControlAppearanceColors.RowHeadHover];

			if (isInvalid)
				return colors[ControlAppearanceColors.RowHeadInvalid];
			
			// all else
			return colors[ControlAppearanceColors.RowHeadNormal];
		}

		/// <summary>
		/// Create default style for grid control.
		/// </summary>
		/// <returns>Default style created</returns>
		public static ControlAppearanceStyle CreateDefaultControlStyle()
		{
			return new ControlAppearanceStyle
			{
				colors = new Dictionary<ControlAppearanceColors, SolidColor>
					{
						{ControlAppearanceColors.LeadHeadNormal, SolidColor.Lavender},
						{ControlAppearanceColors.LeadHeadSelected, SolidColor.Lavender},
						{ControlAppearanceColors.LeadHeadIndicatorStart, SolidColor.Gainsboro},
						{ControlAppearanceColors.LeadHeadIndicatorEnd, SolidColor.Silver},
						{ControlAppearanceColors.ColHeadSplitter, SolidColor.LightSteelBlue},
						{ControlAppearanceColors.ColHeadNormalStart, SolidColor.White},
						{ControlAppearanceColors.ColHeadNormalEnd, SolidColor.Lavender},
						{ControlAppearanceColors.ColHeadHoverStart, SolidColor.LightGoldenrodYellow},
						{ControlAppearanceColors.ColHeadHoverEnd, SolidColor.Goldenrod},
						{ControlAppearanceColors.ColHeadSelectedStart, SolidColor.LightGoldenrodYellow},
						{ControlAppearanceColors.ColHeadSelectedEnd, SolidColor.Goldenrod},
						{ControlAppearanceColors.ColHeadFullSelectedStart, SolidColor.WhiteSmoke},
						{ControlAppearanceColors.ColHeadFullSelectedEnd, SolidColor.LemonChiffon},
						{ControlAppearanceColors.ColHeadSelectedNotFocusedStart, SolidColor.LightGoldenrodYellow},
						{ControlAppearanceColors.ColHeadSelectedNotFocusedEnd, SolidColor.Goldenrod},
						{ControlAppearanceColors.ColHeadFullSelectedNotFocusedStart, SolidColor.WhiteSmoke},
						{ControlAppearanceColors.ColHeadFullSelectedNotFocusedEnd, SolidColor.LemonChiffon},
						{ControlAppearanceColors.ColHeadText, SolidColor.DarkBlue},
						{ControlAppearanceColors.RowHeadSplitter, SolidColor.LightSteelBlue},
						{ControlAppearanceColors.RowHeadNormal, SolidColor.AliceBlue},
						{ControlAppearanceColors.RowHeadHover, SolidColor.LightSteelBlue},
						{ControlAppearanceColors.RowHeadSelected, SolidColor.PaleGoldenrod},
						{ControlAppearanceColors.RowHeadFullSelected, SolidColor.LemonChiffon},
						{ControlAppearanceColors.RowHeadSelectedNotFocused, SolidColor.PaleGoldenrod},
						{ControlAppearanceColors.RowHeadFullSelectedNotFocused, SolidColor.LemonChiffon},
						{ControlAppearanceColors.RowHeadText, SolidColor.DarkBlue},
						{ControlAppearanceColors.GridText, SolidColor.Black},
						{ControlAppearanceColors.GridBackground, SolidColor.White},
						{ControlAppearanceColors.GridLine, SolidColor.FromArgb(255, 208, 215, 229)},
						{ControlAppearanceColors.SelectionBorder, ColorUtility.FromAlphaColor(180, StaticResources.SystemColor_Highlight)},
						{ControlAppearanceColors.SelectionFill, ColorUtility.FromAlphaColor(50, StaticResources.SystemColor_Highlight)},
						{ControlAppearanceColors.SelectionNotFocusedBorder, ColorUtility.FromAlphaColor(180, StaticResources.SystemColor_ControlDark)},
						{ControlAppearanceColors.SelectionNotFocusedFill, ColorUtility.FromAlphaColor(50, StaticResources.SystemColor_ControlDark)},
						{ControlAppearanceColors.HotTrackingBackground, ColorUtility.FromAlphaColor(40, StaticResources.SystemColor_Highlight) },
						{ControlAppearanceColors.HotTrackingNotFocusedBackground, ColorUtility.FromAlphaColor(40, StaticResources.SystemColor_ControlDark) },
						{ControlAppearanceColors.OutlineButtonBorder, SolidColor.Black},
						{ControlAppearanceColors.OutlinePanelBackground, StaticResources.SystemColor_Control},
						{ControlAppearanceColors.OutlinePanelBorder, SolidColor.Silver},
						{ControlAppearanceColors.OutlineButtonText, StaticResources.SystemColor_WindowText},
						{ControlAppearanceColors.SheetTabText, StaticResources.SystemColor_WindowText },
						{ControlAppearanceColors.SheetTabBorder, StaticResources.SystemColor_Highlight },
						{ControlAppearanceColors.SheetTabBackground, SolidColor.White },
						{ControlAppearanceColors.SheetTabSelected, StaticResources.SystemColor_Window },
				},

				SelectionBorderWidth = 3,
			};
		}
	}
	#endregion // Appearance

}
