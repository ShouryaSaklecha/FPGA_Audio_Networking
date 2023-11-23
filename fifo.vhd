library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity fifo is
    generic (
        DATA_WIDTH : positive := 32;
        FIFO_DEPTH : positive := 5
    );
    port (
        clkw    : in  std_logic;
        clkr    : in  std_logic;
        rst     : in  std_logic;
        wr      : in  std_logic;
        rd      : in  std_logic;
        din     : in  std_logic_vector(DATA_WIDTH-1 downto 0);
        empty   : out std_logic;
        full    : out std_logic;
        dout    : out std_logic_vector(DATA_WIDTH-1 downto 0)
    );
end fifo;

architecture arch of fifo is

    type fifo_t is array (0 to 2**FIFO_DEPTH-1) of std_logic_vector(DATA_WIDTH-1 downto 0);
    signal mem : fifo_t;

    signal rdp, wrp : unsigned(FIFO_DEPTH downto 0) := (others => '0');
    signal int_rdp : unsigned(FIFO_DEPTH-1 downto 0);
    signal int_wrp : unsigned(FIFO_DEPTH-1 downto 0);

    signal sig_full : std_logic;
    signal sig_empty : std_logic;
begin

    process(clkw) begin
        if rising_edge(clkw) then
            if wr = '1' then
                mem(to_integer(wrp(FIFO_DEPTH - 1 downto 0))) <= din;
            end if;
        end if;
    end process;

    process(clkw) begin
        if rising_edge(clkw) then
            if rst = '1' then
                wrp <= (others => '0');
            else
                if wr = '1' then
                    wrp <= wrp + 1;
                end if;
            end if;
        end if;
    end process;

    full <= sig_full;
    empty <= sig_empty;
    sig_full <= (rdp(FIFO_DEPTH) xor wrp(FIFO_DEPTH)) when (rdp(FIFO_DEPTH-1 downto 0) = wrp(FIFO_DEPTH-1 downto 0)) else '0';
    sig_empty <= '1' when int_rdp = wrp(FIFO_DEPTH-1 downto 0) else '0';
    int_rdp <= rdp(FIFO_DEPTH-1 downto 0) + 1;
    int_wrp <= wrp(FIFO_DEPTH-1 downto 0);
    dout <= mem(to_integer(int_rdp));

    -- dout <= mem(to_integer(rdp(FIFO_DEPTH-1 downto 0)));
    -- process(clkr) begin
    --     if rising_edge(clkr) then
    --         if rst = '1' then
    --             dout <= (others => '0');
    --         elsif rd = '1' then
    --             dout <= mem(to_integer(rdp(FIFO_DEPTH-1 downto 0)));
    --         end if;
    --     end if;
    -- end process;

    process(clkr) begin 
        if rising_edge(clkr) then
            if rst = '1' then
                rdp <= (others => '0');
            else
                if (rd = '1' and sig_empty = '0') or sig_full = '1' then
                    rdp <= rdp + 1;
                end if;
            end if;
        end if;
    end process;

end arch;
