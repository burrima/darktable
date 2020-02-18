add_cmocka_mock_test(test_filmicrgb
                     SOURCES test_filmicrgb.c ../util/testimg.c
                     LINK_LIBRARIES lib_darktable cmocka
                     MOCKS dt_iop_color_picker_reset)

add_cmocka_mock_test(test_filmicrgb_process
                     SOURCES test_filmicrgb_process.c ../util/testimg.c
                     LINK_LIBRARIES lib_darktable cmocka
                     MOCKS dt_ioppr_get_pipe_work_profile_info)
