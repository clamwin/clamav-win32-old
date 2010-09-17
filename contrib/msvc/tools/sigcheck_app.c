#include <platform.h>
#include <fmap.h>
#include <filetypes.h>
#include <others.h>
#undef main

void formatmessage(int code)
{
    char *message;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, (LPSTR) &message, 0, NULL);
    printf("Sigcheck result: 0x%08x - %s ", code, message);
    LocalFree(message);
}

int main(int argc, char *argv[])
{
    int fd, result;
    cli_ctx ctx;

    if (argc != 2)
    {
        printf("Usage: %s file_to_check\n", argv[0]);
        return 1;
    }

    if ((fd = safe_open(argv[1], O_RDONLY|O_BINARY)) == -1)
    {
        perror("open");
        return 1;
    }

    memset(&ctx, 0, sizeof(cli_ctx));
    ctx.container_type = CL_TYPE_ANY;

    if (!(ctx.fmap = cli_calloc(sizeof(fmap_t *), 1)))
    {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if (!(*ctx.fmap = fmap(fd, 0, 0)))
    {
        fprintf(stderr, "fmap failed\n");
        return 1;
    }

    cl_init(CL_INIT_DEFAULT);
    cl_debug();

    result = cw_sigcheck(&ctx, 0);
    formatmessage(result);
    
    funmap(*ctx.fmap);
    free(ctx.fmap);
    close(fd);
    
    return 0;
}
