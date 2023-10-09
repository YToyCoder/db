cmake_minimum_required(VERSION 3.27)

project(db)

add_compile_options(-g)

add_executable(db)

target_sources(
  db
  PRIVATE 
  main.c
)