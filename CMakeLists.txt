cmake_minimum_required(VERSION 3.16)

project(db)

add_compile_options(-g)

add_executable(db)

target_sources(
  db
  PRIVATE 
  main.c
)

set_target_properties(db PROPERTIES OUTPUT_NAME "db_exe")