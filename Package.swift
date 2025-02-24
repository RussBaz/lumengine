// swift-tools-version: 6.0
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "lumengine",
    products: [
        // Products define the executables and libraries a package produces, making them visible to other packages.
        .library(
            name: "lumengine",
            targets: ["lumengine"]),
    ],
    targets: [
        // Targets are the basic building blocks of a package, defining a module or a test suite.
        // Targets can depend on other targets in this package and products from dependencies.
        .target(
            name: "lumengine",
            dependencies: [
                "cxxLumengine"
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ]
        ),
        .target(
            name: "cxxLumengine",
            cxxSettings: [
                .headerSearchPath("asio/include"),
                .define("ASIO_STANDALONE"),
                .unsafeFlags(["-std=c++20", "-v"])
            ]
        ),
        .testTarget(
            name: "lumengineTests",
            dependencies: ["lumengine"]
        ),
    ],
    cxxLanguageStandard: .cxx20
)
