#!/bin/bash

# Deploy script for Shockee macOS app bundle
# This script creates a complete .app bundle with all Qt dependencies

echo "🚀 Deploying Shockee for macOS..."

# Build the application first
echo "📦 Building application..."
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)

if [ ! -d "Shockee.app" ]; then
    echo "❌ Build failed - Shockee.app not found"
    exit 1
fi

echo "✅ Application built successfully"

# Deploy Qt dependencies
echo "📚 Deploying Qt dependencies..."
if command -v macdeployqt &> /dev/null; then
    macdeployqt Shockee.app -dmg
    echo "✅ Created Shockee.dmg with all dependencies"
else
    echo "⚠️  macdeployqt not found - creating basic app bundle"
    echo "   Install Qt development tools for full deployment"
fi

# Create version info
echo "📝 Creating version info..."
cat > Shockee.app/Contents/Info.plist.version << EOF
Application: Shockee v1.0.0
Built: $(date)
Qt Version: $(qmake -version | grep "Qt version" | cut -d' ' -f4)
Platform: macOS $(sw_vers -productVersion)
Architecture: $(uname -m)
EOF

echo "🎉 Deployment complete!"
echo ""
echo "📁 Output files:"
ls -la *.app *.dmg 2>/dev/null || ls -la *.app

echo ""
echo "🔧 Usage:"
echo "  • Double-click Shockee.app to run"
echo "  • Or: open Shockee.app"
echo "  • Install from: Shockee.dmg (if created)"