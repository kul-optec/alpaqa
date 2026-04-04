using Test
using Alpaqa

@testset "Sanity check" begin
    @test hasproperty(Alpaqa, :run_alpaqa)
end
