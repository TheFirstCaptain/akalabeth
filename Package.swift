// swift-tools-version: 6.0

import PackageDescription

let package = Package(
    name: "Akalabeth",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .executable(name: "AkalabethApp", targets: ["AkalabethApp"]),
        .library(name: "AkalabethMac", targets: ["AkalabethMac"])
    ],
    targets: [
        .target(
            name: "CAkalabeth",
            path: "Core",
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("include")
            ]
        ),
        .target(
            name: "AkalabethMac",
            dependencies: ["CAkalabeth"]
        ),
        .executableTarget(
            name: "AkalabethApp",
            dependencies: ["AkalabethMac", "CAkalabeth"]
        ),
        .testTarget(
            name: "AkalabethMacTests",
            dependencies: ["AkalabethMac", "CAkalabeth"]
        )
    ]
)
