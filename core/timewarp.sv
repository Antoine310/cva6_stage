// Module protection miss/hit

module timewarp
  import ariane_pkg::*;
#(
    parameter config_pkg::cva6_cfg_t CVA6Cfg = config_pkg::cva6_cfg_empty,
    parameter type dcache_req_o_t = logic,
    parameter int HIT_TIME = 10,    // Delais Hit présent 
    parameter int STALL_COMMIT= 10 // temps Stall + 1 
) (
    // Subsystem Clock - SUBSYSTEM
    input logic clk_i,
    // Asynchronous reset active low - SUBSYSTEM
    input logic rst_ni,
    // Lecture du cycle en cours dans csr_regfile 
    input logic csr_lecture_cycle,
    // HIT du Dcache 
    input logic dcache_hit_i,
    // Load commit 
    input logic load_commit_i,
    // Load invalid du commit  
    input logic load_invalid_i,
    // Lock commit 
    output logic protect_en_o
);
    logic [$clog2(HIT_TIME+1)-1:0] compteur_hit;
    logic [$clog2(STALL_COMMIT+1)-1:0] compteur_stall;

    logic hit_en ;
    logic [2:0] dcache_hit_q;


    always_comb begin : activation_stall

        protect_en_o = 1'b0; 
        if (hit_en && csr_lecture_cycle) begin 
            protect_en_o = 1'b1; 
            $display("TIMEWARP PROTECTION ENABLED");

        end else if (compteur_stall !=  0) begin
            protect_en_o = 1'b1;
        end 
    end
    
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (~rst_ni) begin
            hit_en <= 1'b0;
            compteur_hit <= '0;
            compteur_stall <= '0;
            dcache_hit_q <= '0;
        end else begin 
            dcache_hit_q <= dcache_hit_q + dcache_hit_i - ((dcache_hit_q>0) && load_commit_i) - ((dcache_hit_q>0) && load_invalid_i);
            /*
            if(((dcache_hit_q>0) && load_invalid_i) && dcache_hit_i) begin // Un hit + commit hit invalid
                dcache_hit_q <= dcache_hit_q ;
            end else if(((dcache_hit_q>0) && load_commit_i) && dcache_hit_i) begin // Un hit + commit hit valid
                dcache_hit_q <= dcache_hit_q ;
            end else if (dcache_hit_i) begin // Un hit 
                dcache_hit_q <= dcache_hit_q + 1 ;
            end else if ((dcache_hit_q>0) && load_commit_i) begin // Un Commit Hit Valid  
                dcache_hit_q <= dcache_hit_q - 1 ;
            end else if ((dcache_hit_q>0) && load_invalid_i)begin // Un Commit Hit Invalid   
                dcache_hit_q <= dcache_hit_q - 1 ;
            end */

            if ((dcache_hit_q>0) && load_commit_i) begin
                hit_en <= 1'b1; 
                compteur_hit <= HIT_TIME[$bits(compteur_hit)-1:0];
            end else if (compteur_hit != 0) begin
                compteur_hit <= compteur_hit - 1 ; 
                if (compteur_hit == 1) begin
                    hit_en <= 1'b0; 
                end
            end 

            if (hit_en && csr_lecture_cycle) begin 
                hit_en <= 1'b0; 
                compteur_stall <= STALL_COMMIT[$bits(compteur_stall)-1:0];
            end else if (compteur_stall !=  0) begin
                compteur_stall <= compteur_stall - 1 ; 
            end 

             if (csr_lecture_cycle) begin 
                $display("TIMEWARP PROTECTION");
            end
        end 
    end

    logic protect_en_q;
    logic hit_en_q;
    logic csr_cycle_q;
    logic [2:0] dcache_hit_c;    
    int nb_cycle;
    logic load_commit_q;

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            protect_en_q  <= 1'b0;
            hit_en_q      <= 1'b0;
            dcache_hit_c  <= '0;
            csr_cycle_q   <= 1'b0;
            nb_cycle      <= 0;
        end else begin

            if (dcache_hit_c != dcache_hit_q)
                $display("[cycle %0d] dcache_hit_cnt -> %0d", nb_cycle, dcache_hit_q);
        
            if (protect_en_o != protect_en_q)
                $display("[cycle %0d] protect_en_o -> %0b \n", nb_cycle, protect_en_o);

            if (hit_en != hit_en_q)
                $display("[cycle %0d] hit_en -> %0b \n ", nb_cycle, hit_en);

            if (csr_lecture_cycle != csr_cycle_q)
                $display("[cycle %0d] csr_lecture_cycle -> %0b \n" , nb_cycle, csr_lecture_cycle);

            if (load_commit_i != load_commit_q)
                $display("[cycle %0d] load_commit_i -> %0b \n", nb_cycle, load_commit_i);

    
            protect_en_q <= protect_en_o;
            hit_en_q <= hit_en;
            csr_cycle_q <= csr_lecture_cycle;
            dcache_hit_c <= dcache_hit_q;
            load_commit_q <= load_commit_i;
            nb_cycle <= nb_cycle + 1;

        end
    end

endmodule 
