﻿using System;
using System.Drawing;
using Gma.CodeCloud.Controls.Geometry;

namespace Gma.CodeCloud.Controls
{
    public static class LayoutFactory
    {
        public static ILayout CreateLayout(LayoutType layoutType, SizeF size)
        {
            switch (layoutType)
            {
                case LayoutType.Typewriter:
                    return new TypewriterLayout(size);

                case LayoutType.Spiral:
                    return new SpiralLayout(size);
            
                default:
                    throw new ArgumentException(string.Format("No constructor specified to create a layout instance for {0}.", layoutType), "layoutType");
            }
        }
    }
}
