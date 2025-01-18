#include <SFML/Graphics.hpp>
#include <complex>
#include <oneapi/tbb.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <fstream>
#include <string>
#include <filesystem> // For file management (C++17)
#include "benchmark.hpp"

using Complex = std::complex<double>;

int mandelbrot(Complex const& c) {
    int i = 0;
    auto z = c;
    for (; i != 256 && norm(z) < 4.; ++i) {
        z = z * z + c;
    }
    return i;
}

auto to_color(int k) {
    return k < 256 ? sf::Color{static_cast<sf::Uint8>(10 * k), 0, 80}
                   : sf::Color::Black;
}

int main() {
    int const display_width{800};
    int const display_height{800};

    Complex const top_left{-2.2, 1.5};
    Complex const lower_right{0.8, -1.5};
    auto const diff = lower_right - top_left;

    auto const delta_x = diff.real() / display_width;
    auto const delta_y = diff.imag() / display_height;

    sf::Image image;
    image.create(display_width, display_height);

    // Create a csv file to store the results
    std::ofstream result_file("results.csv");
    result_file << "G,Time(s)\n"; // Header

    // For loop on the grainsizes
    std::vector<std::size_t> grainsizes;
    for (int i = 5; i < 800; i++) {
        grainsizes.push_back(i);
    }

    // Store the image files to create the GIF
    std::vector<std::string> image_files;

    // Count the number of tasks created for each grainsize
    std::atomic<int> count{};
    int previous_count = 0; // Store the count from the previous cycle

    for (auto G : grainsizes) {
        count = 0; // Reset count for the current cycle

        //for every grainsize create an image with a superimposed grid showing the task division
        sf::Image grid_image = image;
        
        // Benchmark the parallel_for loop
        auto bench = benchmark([&] {
            tbb::simple_partitioner partitioner{};
            tbb::parallel_for(  
                tbb::blocked_range2d<std::size_t>{0, display_height, G, 0, display_width, G},   // 2D range on the image
                [&](auto const& fr) {
                    ++count;


                    // Compute the Mandelbrot set for this block with a double for loop on the pixels
                    for (int i = static_cast<int>(fr.rows().begin()); i != static_cast<int>(fr.rows().end()); ++i) {
                        for (int j = static_cast<int>(fr.cols().begin()); j != static_cast<int>(fr.cols().end()); ++j) {
                            auto k = mandelbrot(top_left + Complex{delta_x * j, delta_y * i});
                            image.setPixel(j, i, to_color(k));
                            grid_image.setPixel(j, i, to_color(k));
                        }
                    }
                    
                    // Draw grid lines for the grid image block
                    for (int i = static_cast<int>(fr.rows().begin());
                         i != static_cast<int>(fr.rows().end()); ++i) {
                        grid_image.setPixel(static_cast<int>(fr.cols().begin()), i, sf::Color::Red);
                        //grid_image.setPixel(static_cast<int>(fr.cols().end() - 1), i, sf::Color::Red);  // thicker lines if needed
                    }
                    for (int j = static_cast<int>(fr.cols().begin());
                         j != static_cast<int>(fr.cols().end()); ++j) {
                        grid_image.setPixel(j, (fr.rows().begin()), sf::Color::Red);
                        // grid_image.setPixel(j, (fr.rows().end() - 1), sf::Color::Red); // thicker lines if needed
                    }
                },
                partitioner);
        });

        // Save the image only if count changes from the previous cycle
        if (count != previous_count) {
            std::string filename = "mandelbrot_" + std::to_string(G) + ".png";
            grid_image.saveToFile(filename);
            image_files.push_back(filename);
            previous_count = count; // Update the previous count
        }

        // Write the results to the csv file
        result_file << G << "," << bench.count() << "\n";

        // Print the results for every grainsize 
        std::cout << "G " << G << ", " << count << " invocations, " << bench.count()
                  << " s\n";
    }

    result_file.close();

    //Save the mandelbrot image
    image.saveToFile("mandelbrot_par.png");

    // Use ImageMagick to create GIF 
    std::string command = "convert -delay 50 -loop 0";
    for (const auto& file : image_files) {
            command += " " + file;
    }
    command += " mandelbrot_animation.gif";

    int result = std::system(command.c_str());
    if (result == 0) {
        std::cout << "GIF created successfully: mandelbrot_animation.gif\n";
    } else {
        std::cerr << "Error creating GIF\n";
    }

    // Remove all temporary images
    for (const auto& file : image_files) {
        std::filesystem::remove(file);
    }

    return 0;
}