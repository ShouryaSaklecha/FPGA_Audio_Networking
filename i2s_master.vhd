library ieee;
use ieee.std_logic_1164.ALL;
use ieee.numeric_std.ALL;

library work;
use work.aud_param.all;

-- I2S master interface for the SPH0645LM4H MEMs mic
-- useful links:
--   - https://diyi0t.com/i2s-sound-tutorial-for-esp32/
--   - https://cdn-learn.adafruit.com/downloads/pdf/adafruit-i2s-mems-microphone-breakout.pdf
--   - https://cdn-shop.adafruit.com/product-files/3421/i2S+Datasheet.PDF

entity i2s_master is
    generic (
        DATA_WIDTH : natural := 32;
        PCM_PRECISION : natural := 18
    );
    port (
        clk             : in  std_logic;
        clk_1            : in  std_logic;

        -- I2S interface to MEMs mic
        i2s_lrcl        : out std_logic;    -- left/right clk (word sel): 0 = left, 1 = right
        i2s_dout        : in  std_logic;    -- serial data: payload, msb first
        i2s_bclk        : out std_logic;    -- Bit clock: freq = sample rate * bits per channel * number of channels
                                            -- (should run at 2-4MHz). Changes when the next bit is ready.
        -- FIFO interface to MEMs mic
        fifo_din        : out std_logic_vector(DATA_WIDTH - 1 downto 0);
        fifo_w_stb      : out std_logic;    -- Write strobe: 1 = ready to write, 0 = busy
        fifo_full       : in  std_logic     -- 1 = not full, 0 = full
    );
    
end i2s_master;

architecture Behavioral of i2s_master is
    constant BCLK_DIVIDER : natural := 18; -- Number of cycles of 100MHz clock per bit block
    constant LRCL_DIVIDER: natural := 31;  -- Number of cyclyes of bit clock per word clock

    -- Internal signals for BCLK generation
    signal bclk_counter : natural range 0 to BCLK_DIVIDER := 0;
    signal bclk : std_logic := '0';
    
    -- Signal for LRCL 
    signal lrcl_counter : natural range 0 to LRCL_DIVIDER := 0; 
    signal lrcl : std_logic := '0';
    
    --Signal to take in serial bits 
    signal data_in: std_logic_vector (31 downto 0) := "00000000000000000000000000000000";
    signal bit_count: natural range 0 to 31 := 0;
    signal update_data: std_logic := '0';
    type state1_type is (read, idle);
    type state2_type is (set, waiting, hold);
    signal state: state1_type := read;
    signal fifo_state: state2_type := waiting;
    
begin

    -- Bit clock counter
    -- Calculated as clk / (Samples Rate [44100] * Num Channels [2] * Bit Rate [32]) 
    -- then rounded to 36 multiplied by duty cycle (0.5) to get to 18
    process (clk)
    begin
        if rising_edge(clk) then
        bclk_counter <= bclk_counter + 1;
            if bclk_counter = 18 then
                bclk <= not bclk;
                bclk_counter <= 0;
            end if;
        end if;
    end process;
    i2s_bclk <= bclk;
    
    -- Word select clock
    -- Calculated as bit clock / 64
    -- Multiplied by duty cycle (0.5) to get to 32
    process (bclk)
    begin
        if rising_edge(bclk) then
            lrcl_counter <= lrcl_counter + 1;
            if lrcl_counter = 31 then
                lrcl <= not lrcl;
                lrcl_counter <= 0;
            end if;
        end if;
    end process;
    i2s_lrcl <= lrcl;
    

    -- State machine for the bit clock
    -- Counts the first 18 bits then sits idle until we've counted a full 32 bits
    process(bclk)
    begin
        if rising_edge(bclk) then 
            bit_count <= bit_count + 1; 
            case state is
                when read =>
                    if bit_count >= 18 then
                        state <= idle;
                    else
                        update_data <= '1';
                        state <= read;
                    end if;
                when idle =>
                    update_data  <= '0';
                    if bit_count = 31 then
                        state <= read;
                        bit_count <= 0;
                    else
                        state <= idle;
                    end if;            
            end case;            
       end if;          
    end process;
    
    -- Output logic for state machine
    -- If we're reading we add the data to data_in, otherwise we do nothing
    process(bclk)
    begin
        if rising_edge (bclk) then
            if update_data = '1' then
               data_in <= i2s_dout & data_in(31 downto 1);  -- load dout into buffer
            end if;
        end if;
    end process;
    fifo_din <= "00000000000000" & data_in(31 downto 14); 
    
    
    -- FIFO / I2S handshake
    -- Whenever we've recorded 32 bits we strobe the buffer
    -- Also checks if buffer is full and doesn't write if it is
    process(clk)
    begin
        if rising_edge (clk) then
            
            case fifo_state is
                when waiting =>
                fifo_w_stb <= '0';
                    if state = read then
                        fifo_state <= waiting;
                    else
                        fifo_state <= set;
                    end if;
                when set =>
                    if fifo_full = '1' then
                        -- fifo is full, discard the buffer
                        fifo_w_stb <= '0'; -- set to busy
                    else 
                        fifo_w_stb <= '1';
                    end if;
                    fifo_state <= hold;
                when hold =>
                    fifo_w_stb <= '0';
                if state = read then
                    fifo_state <= waiting;
                else
                    fifo_state <= hold;
                end if;
            end case;
        end if;
    end process;
          
end Behavioral;
