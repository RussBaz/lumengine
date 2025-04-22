// swift-tools-version: 6.1

import PackageDescription

let package = Package(
    name: "lumengine",
    platforms: [
        .macOS(.v14),
        .iOS(.v17),
    ],
    products: [
        .library(
            name: "lumengine",
            targets: ["lumengine"]
        ),
    ],
    targets: [
        // Header only dependency Asio (Non-Boost)
        // Repository: https://github.com/chriskohlhoff/asio
        // Commit SHA: 4730c543ba2b8f9396ef1964a25ccc26eb1aea64
        .target(
            name: "cxxAsio",
            cxxSettings: [
                .define("ASIO_STANDALONE"),
            ]
        ),
        // Custom CPP server implementation that uses Asio under the hood
        .target(
            name: "cxxLumengine",
            dependencies: [
                "cxxAsio"
            ]
        ),
        .target(
            name: "lumengine",
            dependencies: [
                "cxxLumengine",
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
            ]
        ),
        .testTarget(
            name: "lumengineTests",
            dependencies: ["lumengine"],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
            ]
        ),
    ],
    cxxLanguageStandard: .cxx20
)
